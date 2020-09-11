/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "KernelGenerator.h"

#include <arm_compute/runtime/CL/CLFunctions.h>   // Include all ARM Compute CL functions
#include <arm_compute/runtime/CL/CLFunctionsEx.h> // Include all ARM Compute EX CL functions

#include <AclActivationBuilder.h>
#include <AclFunction.h>
#include <Convert.h>
#include <Swizzle.h>

#include "ir/Index.h"
#include "ir/DataType.h"
#include "ir/InternalType.h"
#include "exec/NopFunction.h"
#include "exec/FunctionSequence.h"
#include "util/logging.h"
#include "util/Utils.h"
#include "AclKernelGen.h"

namespace onert
{
namespace backend
{
namespace acl_cl
{

using ::onert::backend::acl_common::asAclFunction;
using ActivationBuilder = ::onert::backend::acl_common::AclActivationBuilder<
    ::arm_compute::ICLTensor, ::arm_compute::CLActivationLayer, acl_common::AclFunction>;

KernelGenerator::KernelGenerator(
    const ir::Operands &operands_ctx, const ir::Operations &operations_ctx,
    const std::shared_ptr<TensorBuilder> &tensor_builder,
    const std::shared_ptr<acl_common::AclTensorRegistry<TensorManager>> &tensor_reg)
    : _ctx(operands_ctx), _operations_ctx(operations_ctx), _tensor_builder(tensor_builder),
      _tensor_reg(tensor_reg), _current_op_seq_layout(ir::Layout::UNKNOWN)
{
  // DO NOTHING
}

void KernelGenerator::visit(const ir::OpSequence &op_seq)
{
  // TODO Move this to IKernelGenerator
  //      (all derivatives have the same implementation for this)
  assert(!_return_fn_seq);
  _return_fn_seq = std::make_unique<exec::FunctionSequence>();
  _return_fn_seq->enableDynamicShapeInferer(false);

  _current_op_seq_layout = op_seq.getLayout();
  for (const auto &operation_idx : op_seq.operations())
  {
    const auto &node = _operations_ctx.at(operation_idx);
    node.accept(*this);
    _return_fn_seq->append(releaseFunction());
  }
}

void KernelGenerator::visit(const ir::operation::BatchToSpaceND &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::BatchToSpaceND::Input::INPUT)};
  const auto block_size_index{
      node.getInputs().at(ir::operation::BatchToSpaceND::Input::BLOCK_SIZE)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto block_size_tensor = _tensor_reg->getAclTensor(block_size_index).get();

  assert(_ctx.at(block_size_index).data());

  auto fn = acl_common::generateLayer<arm_compute::CLBatchToSpaceLayer>(
      ifm_tensor->handle(), block_size_tensor->handle(), ofm_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::BinaryArithmetic &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto lhs_index{node.getInputs().at(ir::operation::BinaryArithmetic::Input::LHS)};
  const auto rhs_index{node.getInputs().at(ir::operation::BinaryArithmetic::Input::RHS)};

  const auto activation = node.param().activation;

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto lhs_tensor = _tensor_reg->getAclTensor(lhs_index).get();
  auto rhs_tensor = _tensor_reg->getAclTensor(rhs_index).get();

  const auto act_info = acl_common::asActivationLayerInfo(activation);

  std::unique_ptr<arm_compute::IFunction> fn;
  switch (node.param().arithmetic_type)
  {
    case ir::operation::BinaryArithmetic::ArithmeticType::ADD:
    {
      fn = acl_common::generateLayer<arm_compute::CLArithmeticAddition>(
          lhs_tensor->handle(), rhs_tensor->handle(), ofm_tensor->handle(),
          arm_compute::ConvertPolicy::SATURATE, act_info);
      break;
    }
    case ir::operation::BinaryArithmetic::ArithmeticType::SUB:
    {
      fn = acl_common::generateLayer<arm_compute::CLArithmeticSubtraction>(
          lhs_tensor->handle(), rhs_tensor->handle(), ofm_tensor->handle(),
          arm_compute::ConvertPolicy::SATURATE, act_info);
      break;
    }
    case ir::operation::BinaryArithmetic::ArithmeticType::MUL:
    {
      fn = acl_common::generateLayer<arm_compute::CLPixelWiseMultiplication>(
          lhs_tensor->handle(), rhs_tensor->handle(), ofm_tensor->handle(), 1.0, // scale
          arm_compute::ConvertPolicy::SATURATE, arm_compute::RoundingPolicy::TO_NEAREST_EVEN,
          act_info);
      break;
    }
    case ir::operation::BinaryArithmetic::ArithmeticType::DIV:
    {
      fn = acl_common::generateLayer<arm_compute::CLArithmeticDivision>(
          lhs_tensor->handle(), rhs_tensor->handle(), ofm_tensor->handle(), act_info);
      break;
    }
    default:
      assert(false && "The BinaryArithmetic operation supports only binary arithmetic operations");
      break;
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Conv2D &node)
{
  using ir::operation::Conv2D;

  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(Conv2D::Input::INPUT)};
  const auto ker_index{node.getInputs().at(Conv2D::Input::KERNEL)};
  const auto bias_index{node.getInputs().at(Conv2D::Input::BIAS)};

  const auto ifm_shape = _ctx.at(ifm_index).shape().asFeature(_current_op_seq_layout);
  const auto ofm_shape = _ctx.at(ofm_index).shape().asFeature(_current_op_seq_layout);
  // Kernel format is [depth_out, kernel_height, kernel_width, depth_in].
  const auto &ker_shape = _ctx.at(ker_index).shape();
  const auto ker_height = ker_shape.dim(1);
  const auto ker_width = ker_shape.dim(2);

  const auto stride = node.param().stride;
  const auto padding = ir::calculatePadding(node.param().padding, ifm_shape, ofm_shape, stride,
                                            ker_width, ker_height);
  const auto activation = node.param().activation;

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto ker_tensor = _tensor_reg->getAclTensor(ker_index).get();
  auto bias_tensor = _tensor_reg->getAclTensor(bias_index).get();

  const auto conv_info = acl_common::asPadStrideInfo(padding, stride);
  const auto act_info = acl_common::asActivationLayerInfo(activation);

  auto fn = acl_common::generateLayer<arm_compute::CLConvolutionLayer>(
      _tensor_builder->acl_tensor_manager()->internal_buffer_manager(), ifm_tensor->handle(),
      ker_tensor->handle(), bias_tensor->handle(), ofm_tensor->handle(), conv_info,
      ::arm_compute::WeightsInfo(), ::arm_compute::Size2D(1U, 1U), act_info);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::DepthwiseConv2D &node)
{
  using ir::operation::DepthwiseConv2D;

  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(DepthwiseConv2D::Input::INPUT)};
  const auto ker_index{node.getInputs().at(DepthwiseConv2D::Input::KERNEL)};
  const auto bias_index{node.getInputs().at(DepthwiseConv2D::Input::BIAS)};

  const auto ifm_shape = _ctx.at(ifm_index).shape().asFeature(_current_op_seq_layout);
  const auto ofm_shape = _ctx.at(ofm_index).shape().asFeature(_current_op_seq_layout);
  // Kernel format is [1, kernel_height, kernel_width, depth_out].
  const auto &ker_shape = _ctx.at(ker_index).shape();
  const auto ker_height = ker_shape.dim(1);
  const auto ker_width = ker_shape.dim(2);

  const auto stride = node.param().stride;
  const auto padding = ir::calculatePadding(node.param().padding, ifm_shape, ofm_shape, stride,
                                            ker_width, ker_height);
  const auto multiplier = node.param().multiplier;
  const auto activation = node.param().activation;

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto ker_tensor = _tensor_reg->getAclTensor(ker_index).get();
  auto bias_tensor = _tensor_reg->getAclTensor(bias_index).get();

  const auto conv_info = acl_common::asPadStrideInfo(padding, stride);
  const auto act_info = acl_common::asActivationLayerInfo(activation);

  {
    auto fn = acl_common::generateLayer<arm_compute::CLDepthwiseConvolutionLayer>(
        ifm_tensor->handle(), ker_tensor->handle(), bias_tensor->handle(), ofm_tensor->handle(),
        conv_info, multiplier, act_info);

    _return_fn = asAclFunction(std::move(fn));
  }
}

void KernelGenerator::visit(const ir::operation::Concat &node)
{
  const auto ofm_index{node.getOutputs().at(0)};

  std::vector<ir::OperandIndex> input_indexes;

  for (const auto &input : node.getInputs())
    input_indexes.emplace_back(input);

  const auto axis = node.param().axis;

  // Concat elimination check
  bool eliminated = _tensor_builder->areSubTensorsOf(ofm_index, node.getInputs());
  if (eliminated)
  {
    // If concat eliminated, return a NOP IFunction
    VERBOSE(acl_cl_KernelGenerator_Concat) << "Concat eliminated" << std::endl;
    _return_fn = std::make_unique<exec::NopFunction>();
    return;
  }

  auto output_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  std::vector<::arm_compute::ICLTensor *> input_tensors;
  for (auto &ifm_ind : input_indexes)
    input_tensors.emplace_back(_tensor_reg->getAclTensor(ifm_ind)->handle());

  std::unique_ptr<::arm_compute::IFunction> fn;
  if (input_indexes.size() < 2)
  {
    fn = acl_common::generateLayer<arm_compute::CLCopy>(input_tensors.at(0),
                                                        output_tensor->handle());
  }
  else
  {
    const auto rank = _ctx.at(ofm_index).shape().rank();
    const auto frontend_layout = _current_op_seq_layout;
    const auto backend_layout = output_tensor->layout();
    const auto fixed_axis =
        acl_common::ToARMComputeAxis(rank, axis, frontend_layout, backend_layout).value();
    fn = acl_common::generateLayer<::arm_compute::CLConcatenateLayer>(
        input_tensors, output_tensor->handle(), fixed_axis);
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::FullyConnected &node)
{
  const auto output_index{node.getOutputs().at(0)};
  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  const auto activation = node.param().activation;

  auto fn = acl_common::kernelGenFullyConnected<acl_common::AclFunction, ::arm_compute::ICLTensor,
                                                ::arm_compute::CLFullyConnectedReshapingLayer>(
      node, _ctx, _tensor_builder, _tensor_reg, _current_op_seq_layout);
  _return_fn = std::make_unique<exec::FunctionSequence>(
      std::move(fn), ActivationBuilder::generate(activation, output_tensor->handle()));
}

void KernelGenerator::visit(const ir::operation::Reduce &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::Reduce::Input::INPUT)};
  const auto axes_index{node.getInputs().at(ir::operation::Reduce::Input::AXES)};
  const auto keep_dims{node.param().keep_dims};
  const auto reduce_type = node.param().reduce_type;

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  // Convert to ACL axes taking into account negative values and possible duplicates.
  const auto &axes = _ctx.at(axes_index);
  const auto input_rank = _ctx.at(input_index).shape().rank();
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = input_tensor->layout();

  std::unique_ptr<arm_compute::IFunction> fn;
  if (reduce_type == ir::operation::Reduce::ReduceType::MEAN)
  {
    const auto acl_axes =
        acl_common::asCoordinates(axes, input_rank, frontend_layout, backend_layout);
    fn = acl_common::generateLayer<arm_compute::CLReduceMean>(input_tensor->handle(), acl_axes,
                                                              keep_dims, output_tensor->handle());
  }
  else
  {
    const auto acl_axes = acl_common::asSet(axes, input_rank, frontend_layout, backend_layout);

    fn = acl_common::generateLayer<arm_compute::CLReduceOperation>(
        _tensor_builder->acl_tensor_manager()->internal_buffer_manager(), input_tensor->handle(),
        output_tensor->handle(), acl_axes, keep_dims, acl_common::convertReduceType(reduce_type));
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Reshape &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::Reshape::Input::INPUT)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  // NOTE This operation must not be changed the layout from frontend to backend
  //      So, PermutationOperationPass makes layouts of frontend and backend the same.
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = output_tensor->layout();
  assert((_ctx.at(input_index).shape().rank() < 4 && _ctx.at(output_index).shape().rank() < 4) ||
         frontend_layout == backend_layout);
  UNUSED_RELEASE(frontend_layout);
  UNUSED_RELEASE(backend_layout);

  auto fn = acl_common::generateLayer<arm_compute::CLReshapeLayer>(input_tensor->handle(),
                                                                   output_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Squeeze &node)
{
  // Squeeze is identical to reshape except that it has an optional dimensions input.
  // In addition, optional dims_index is ignored since output tensor already has squeezed shape
  // by freezer and toco
  // TODO Support multi-layout for frontend and backend
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::Squeeze::Input::INPUT)};
  const auto dims{node.param().dims};
  const auto ndim{node.param().ndim};
  (void)dims;
  (void)ndim;

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();
  auto fn = acl_common::generateLayer<arm_compute::CLReshapeLayer>(input_tensor->handle(),
                                                                   output_tensor->handle());
  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Softmax &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::Softmax::Input::INPUT)};

  const auto beta = node.param().beta;

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLSoftmaxLayer>(
      _tensor_builder->acl_tensor_manager()->internal_buffer_manager(), input_tensor->handle(),
      output_tensor->handle(), beta);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Slice &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::Slice::Input::INPUT)};
  const auto begins_index{node.getInputs().at(ir::operation::Slice::Input::BEGINS)};
  const auto sizes_index{node.getInputs().at(ir::operation::Slice::Input::SIZES)};

  auto outputData_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto inputData_tensor = _tensor_reg->getAclTensor(input_index).get();
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = inputData_tensor->layout();

  // Set initializers for indices data such as order of inputData
  int input_rank = _ctx.at(input_index).shape().rank();
  std::vector<int32_t> starts;
  std::vector<int32_t> ends;
  starts.resize(input_rank, 0);
  ends.resize(input_rank, 0);
  {
    assert(_ctx.at(begins_index).data());
    assert(_ctx.at(sizes_index).data());
    auto beginData_base = _ctx.at(begins_index).data()->base();
    auto sizeData_base = _ctx.at(sizes_index).data()->base();
    const int beginData_size = _ctx.at(begins_index).shape().num_elements();
    const int sizeData_size = _ctx.at(sizes_index).shape().num_elements();

    using ir::DataType;

    UNUSED_RELEASE(beginData_size);
    UNUSED_RELEASE(sizeData_size);

    assert(_ctx.at(begins_index).typeInfo().type() == DataType::INT32);
    assert(_ctx.at(sizes_index).typeInfo().type() == DataType::INT32);
    assert(beginData_size == input_rank);
    assert(sizeData_size == input_rank);

    assert(beginData_base != nullptr);
    for (int n = 0; n < input_rank; ++n)
    {
      auto axis = ::onert::backend::acl_common::ToARMComputeAxis(input_rank, n, frontend_layout,
                                                                 backend_layout)
                      .value();

      int32_t begin_value = *(reinterpret_cast<const int32_t *>(beginData_base) + n);
      starts[axis] = begin_value;

      int32_t size_value = *(reinterpret_cast<const int32_t *>(sizeData_base) + n);
      ends[axis] = begin_value + size_value;
    }
  }

  ::arm_compute::Coordinates starts_set;
  ::arm_compute::Coordinates ends_set;

  for (size_t i = 0; i < starts.size(); ++i)
  {
    starts_set.set(i, starts[i]);
    ends_set.set(i, ends[i]);
  }

  auto fn = acl_common::generateLayer<arm_compute::CLSlice>(
      inputData_tensor->handle(), outputData_tensor->handle(), starts_set, ends_set);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::StridedSlice &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::StridedSlice::Input::INPUT)};
  const auto starts_index{node.getInputs().at(ir::operation::StridedSlice::Input::STARTS)};
  const auto ends_index{node.getInputs().at(ir::operation::StridedSlice::Input::ENDS)};
  const auto strides_index{node.getInputs().at(ir::operation::StridedSlice::Input::STRIDES)};

  auto outputData_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto inputData_tensor = _tensor_reg->getAclTensor(input_index).get();
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = inputData_tensor->layout();

  // Set initializers for indices data such as order of inputData
  int input_rank = _ctx.at(input_index).shape().rank();
  std::vector<int32_t> starts;
  std::vector<int32_t> ends;
  std::vector<int32_t> strides;
  starts.resize(input_rank, 0);
  ends.resize(input_rank, 0);
  strides.resize(input_rank, 0);
  {
    assert(_ctx.at(starts_index).data());
    assert(_ctx.at(ends_index).data());
    assert(_ctx.at(strides_index).data());
    auto startData_base = _ctx.at(starts_index).data()->base();
    auto endData_base = _ctx.at(ends_index).data()->base();
    auto stridesData_base = _ctx.at(strides_index).data()->base();
    const int startData_size = _ctx.at(starts_index).shape().num_elements();
    const int endData_size = _ctx.at(ends_index).shape().num_elements();
    const int stridesData_size = _ctx.at(strides_index).shape().num_elements();

    using ir::DataType;

    UNUSED_RELEASE(startData_size);
    UNUSED_RELEASE(endData_size);
    UNUSED_RELEASE(stridesData_size);

    assert(_ctx.at(starts_index).typeInfo().type() == DataType::INT32);
    assert(_ctx.at(ends_index).typeInfo().type() == DataType::INT32);
    assert(_ctx.at(strides_index).typeInfo().type() == DataType::INT32);
    assert(startData_size == input_rank);
    assert(endData_size == input_rank);
    assert(stridesData_size == input_rank);

    assert(startData_base != nullptr);
    for (int n = 0; n < input_rank; ++n)
    {
      auto axis = ::onert::backend::acl_common::ToARMComputeAxis(input_rank, n, frontend_layout,
                                                                 backend_layout)
                      .value();

      int32_t start_value = *(reinterpret_cast<const int32_t *>(startData_base) + n);
      starts[axis] = start_value;

      int32_t end_value = *(reinterpret_cast<const int32_t *>(endData_base) + n);
      ends[axis] = end_value;

      int32_t strides_value = *(reinterpret_cast<const int32_t *>(stridesData_base) + n);
      strides[axis] = strides_value;
    }
  }

  // Set mask bits such as order of inputData
  const auto begin_mask = acl_common::ReorderBits<int32_t>(node.param().begin_mask, input_rank,
                                                           frontend_layout, backend_layout);
  const auto end_mask = acl_common::ReorderBits<int32_t>(node.param().end_mask, input_rank,
                                                         frontend_layout, backend_layout);
  const auto shrink_axis_mask = acl_common::ReorderBits<int32_t>(
      node.param().shrink_axis_mask, input_rank, frontend_layout, backend_layout);

  ::arm_compute::Coordinates starts_set;
  ::arm_compute::Coordinates ends_set;
  ::arm_compute::BiStrides strides_set;

  for (size_t i = 0; i < starts.size(); ++i)
  {
    starts_set.set(i, starts[i]);
    ends_set.set(i, ends[i]);
    strides_set.set(i, strides[i]);
  }

  // Disable applied dim_correction
  const auto orig_input_shape = inputData_tensor->info()->tensor_shape();
  if (starts_set.num_dimensions() != inputData_tensor->info()->num_dimensions())
  {
    // This means that high dimension's value is 1 and input tensor is applied dim_correction
    const auto input = _ctx.at(input_index);
    inputData_tensor->info()->set_tensor_shape(
        acl_common::asTensorShape(input.shape(), _current_op_seq_layout, backend_layout, false));
  }

  auto fn = acl_common::generateLayer<arm_compute::CLStridedSlice>(
      inputData_tensor->handle(), outputData_tensor->handle(), starts_set, ends_set, strides_set,
      begin_mask, end_mask, shrink_axis_mask);

  // Revert disabling applied dim_correction
  inputData_tensor->info()->set_tensor_shape(orig_input_shape);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Transpose &node)
{
  const auto ofm_idx{node.getOutputs().at(0)};
  const auto ifm_idx{node.getInputs().at(ir::operation::Transpose::Input::INPUT)};
  const auto perm_idx{node.getInputs().at(ir::operation::Transpose::Input::PERMUTATION)};

  const auto rank = _ctx.at(ifm_idx).shape().rank();

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_idx).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_idx).get();
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = ifm_tensor->layout();

  const auto perms = _ctx.at(perm_idx);
  std::vector<int32_t> pv;
  if (perms.shape() == ir::Shape{0})
  {
    pv.resize(rank);
    std::iota(pv.begin(), pv.end(), 0);
    std::reverse(pv.begin(), pv.end());
  }
  else
  {
    pv = _ctx.at(perm_idx).asVector<int32_t>();
  }

  std::unique_ptr<arm_compute::IFunction> fn;
  if (rank == 1)
  {
    fn = acl_common::generateLayer<arm_compute::CLCopy>(ifm_tensor->handle(), ofm_tensor->handle());
  }
  else if (rank == 2)
  {
    assert(pv.size() == 2 && pv.at(0) == 1 && pv.at(1) == 0);
    fn = acl_common::generateLayer<arm_compute::CLTranspose>(ifm_tensor->handle(),
                                                             ofm_tensor->handle());
  }
  else
  {
    auto backend_pv =
        acl_common::getARMComputePermutationVector(rank, pv, frontend_layout, backend_layout);

    fn = acl_common::generateLayer<arm_compute::CLPermute>(ifm_tensor->handle(),
                                                           ofm_tensor->handle(), backend_pv);
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ElementwiseActivation &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ElementwiseActivation::Input::INPUT)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  const ::arm_compute::ActivationLayerInfo act_info = acl_common::asActivationLayerInfo(
      node.param().op_type, node.param().alpha, node.param().beta);

  auto fn = acl_common::generateLayer<arm_compute::CLActivationLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), act_info);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ElementwiseBinary &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto lhs_index{node.getInputs().at(ir::operation::ElementwiseBinary::Input::LHS)};
  const auto rhs_index{node.getInputs().at(ir::operation::ElementwiseBinary::Input::RHS)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto lhs_tensor = _tensor_reg->getAclTensor(lhs_index).get();
  auto rhs_tensor = _tensor_reg->getAclTensor(rhs_index).get();

  std::unique_ptr<arm_compute::IFunction> fn;
  switch (node.param().op_type)
  {
    case ir::operation::ElementwiseBinary::ElementwiseBinaryType::LOGICAL_AND:
    {
      fn = acl_common::generateLayer<arm_compute::CLBinaryLogicalOp>(
          lhs_tensor->handle(), rhs_tensor->handle(), output_tensor->handle(),
          arm_compute::BinaryLogicalOperation::AND);
      break;
    }
    case ir::operation::ElementwiseBinary::ElementwiseBinaryType::LOGICAL_OR:
    {
      fn = acl_common::generateLayer<arm_compute::CLBitwiseOr>(
          lhs_tensor->handle(), rhs_tensor->handle(), output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseBinary::ElementwiseBinaryType::MAX:
    {
      fn = acl_common::generateLayer<arm_compute::CLElementwiseMax>(
          lhs_tensor->handle(), rhs_tensor->handle(), output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseBinary::ElementwiseBinaryType::MIN:
    {
      fn = acl_common::generateLayer<arm_compute::CLElementwiseMin>(
          lhs_tensor->handle(), rhs_tensor->handle(), output_tensor->handle());
      break;
    }
    default:
    {
      std::string err_msg("acl_cl KernelGenerator : " + node.name() +
                          "is not elementwise-binary operations");
      assert(false && err_msg.c_str());
      break;
    }
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ElementwiseUnary &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::ElementwiseUnary::Input::INPUT)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  std::unique_ptr<arm_compute::IFunction> fn;
  switch (node.param().op_type)
  {
    case ir::operation::ElementwiseUnary::Type::ABS:
    {
      const ::arm_compute::ActivationLayerInfo act_info{
          ::arm_compute::ActivationLayerInfo::ActivationFunction::ABS};

      fn = acl_common::generateLayer<arm_compute::CLActivationLayer>(
          input_tensor->handle(), output_tensor->handle(), act_info);
      break;
    }
    case ir::operation::ElementwiseUnary::Type::CAST:
    {
      if (input_tensor->data_type() == output_tensor->data_type())
      {
        fn = acl_common::generateLayer<arm_compute::CLCopy>(input_tensor->handle(),
                                                            output_tensor->handle());
        ;
      }
      else
      {
        // TODO Support converting float to int32 as round down
        fn = acl_common::generateLayer<arm_compute::CLCast>(
            input_tensor->handle(), output_tensor->handle(), arm_compute::ConvertPolicy::SATURATE);
      }
      break;
    }
    case ir::operation::ElementwiseUnary::Type::DEQUANTIZE:
    {
      fn = acl_common::generateLayer<arm_compute::CLDequantizationLayer>(input_tensor->handle(),
                                                                         output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::EXP:
    {
      fn = acl_common::generateLayer<arm_compute::CLExpLayer>(input_tensor->handle(),
                                                              output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::FLOOR:
    {
      fn = acl_common::generateLayer<arm_compute::CLFloor>(input_tensor->handle(),
                                                           output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::LOGICAL_NOT:
    {
      fn = acl_common::generateLayer<arm_compute::CLBitwiseNot>(input_tensor->handle(),
                                                                output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::NEG:
    {
      fn = acl_common::generateLayer<arm_compute::CLNeg>(input_tensor->handle(),
                                                         output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::RSQRT:
    {
      fn = acl_common::generateLayer<arm_compute::CLRsqrtLayer>(input_tensor->handle(),
                                                                output_tensor->handle());
      break;
    }
    case ir::operation::ElementwiseUnary::Type::SQRT:
    {
      const ::arm_compute::ActivationLayerInfo act_info{
          ::arm_compute::ActivationLayerInfo::ActivationFunction::SQRT};

      fn = acl_common::generateLayer<arm_compute::CLActivationLayer>(
          input_tensor->handle(), output_tensor->handle(), act_info);
      break;
    }
    default:
    {
      throw std::runtime_error("acl_cl KernelGenerator : " + node.name() + "is not supported yet");
      break;
    }
  }

  auto acl_fn = asAclFunction(std::move(fn));

  _return_fn = std::move(acl_fn);
}

void KernelGenerator::visit(const ir::operation::ExpandDims &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::ExpandDims::Input::INPUT)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLReshapeLayer>(input_tensor->handle(),
                                                                   output_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::InstanceNorm &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::InstanceNorm::Input::INPUT)};
  const auto gamma_index{node.getInputs().at(ir::operation::InstanceNorm::Input::GAMMA)};
  const auto beta_index{node.getInputs().at(ir::operation::InstanceNorm::Input::BETA)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto gamma_tensor = _tensor_reg->getAclTensor(gamma_index).get();
  auto beta_tensor = _tensor_reg->getAclTensor(beta_index).get();
  auto epsilon = node.param().epsilon;
  auto activation = node.param().activation;

  auto fn = acl_common::generateLayer<arm_compute::CLInstanceNormalizationLayerEx>(
      ifm_tensor->handle(), ofm_tensor->handle(), gamma_tensor->handle(), beta_tensor->handle(),
      epsilon);

  _return_fn = std::make_unique<exec::FunctionSequence>(
      asAclFunction(std::move(fn)), ActivationBuilder::generate(activation, ofm_tensor->handle()));
}

void KernelGenerator::visit(const ir::operation::LSTM &node)
{
  _return_fn = acl_common::kernelGenLSTM<acl_common::AclFunction, ::arm_compute::ICLTensor,
                                         ::arm_compute::CLLSTMLayer>(node, _ctx, _tensor_reg);
}

void KernelGenerator::visit(const ir::operation::Comparison &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input0_index{node.getInputs().at(ir::operation::Comparison::Input::INPUT0)};
  const auto input1_index{node.getInputs().at(ir::operation::Comparison::Input::INPUT1)};

  const auto comparison_type = node.param().comparison_type;

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input0_tensor = _tensor_reg->getAclTensor(input0_index).get();
  auto input1_tensor = _tensor_reg->getAclTensor(input1_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLComparison>(
      input0_tensor->handle(), input1_tensor->handle(), output_tensor->handle(),
      (arm_compute::ComparisonOperation)comparison_type);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::OneHot &node)
{
  const auto output_idx{node.getOutputs().at(0)};
  const auto indices_idx{node.getInputs().at(ir::operation::OneHot::Input::INDICES)};
  const auto depth_idx{node.getInputs().at(ir::operation::OneHot::Input::DEPTH)};
  const auto onvalue_idx{node.getInputs().at(ir::operation::OneHot::Input::ON_VALUE)};
  const auto offvalue_idx{node.getInputs().at(ir::operation::OneHot::Input::OFF_VALUE)};
  const auto depth = _ctx.at(depth_idx).asScalar<int32_t>();
  assert(depth > 0);

  auto output_tensor = _tensor_reg->getAclTensor(output_idx).get();
  auto indices_tensor = _tensor_reg->getAclTensor(indices_idx).get();
  auto onvalue_tensor = _tensor_reg->getAclTensor(onvalue_idx).get();
  auto offvalue_tensor = _tensor_reg->getAclTensor(offvalue_idx).get();

  const size_t output_rank = _ctx.at(output_idx).shape().rank();
  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = output_tensor->layout();
  int32_t axis = node.param().axis == -1 ? output_rank - 1 : node.param().axis;
  axis = acl_common::ToARMComputeAxis(output_rank, axis, frontend_layout, backend_layout).value();

  const auto orig_output_acl_tensor_shape = output_tensor->info()->tensor_shape();
  if (output_rank != output_tensor->info()->num_dimensions())
  {
    // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
    output_tensor->info()->set_tensor_shape(acl_common::asTensorShape(
        _ctx.at(output_idx).shape(), _current_op_seq_layout, backend_layout, false));
  }

  auto fn = acl_common::generateLayer<arm_compute::CLOneHot>(
      indices_tensor->handle(), onvalue_tensor->handle(), offvalue_tensor->handle(),
      output_tensor->handle(), static_cast<uint32_t>(depth), axis);

  output_tensor->info()->set_tensor_shape(orig_output_acl_tensor_shape);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Pack &node)
{
  const auto output_index{node.getOutputs().at(0)};
  auto axis{node.param().axis};

  const auto output_rank = _ctx.at(output_index).shape().rank();

  std::vector<ir::OperandIndex> input_indexes;
  for (const auto &input_index : node.getInputs())
    input_indexes.emplace_back(input_index);

  auto output = _tensor_reg->getAclTensor(output_index).get()->handle();
  std::vector<arm_compute::ICLTensor *> inputs;
  for (const auto &input_index : input_indexes)
    inputs.emplace_back(_tensor_reg->getAclTensor(input_index)->handle());

  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = _tensor_reg->getAclTensor(output_index).get()->layout();

  if (axis < 0)
    axis += output_rank;
  axis = acl_common::ToARMComputeAxis(output_rank, axis, frontend_layout, backend_layout).value();

  // Disable applied dim_correction
  std::vector<arm_compute::TensorShape> orig_inputs_acl_tensor_shapes;
  for (const auto &input_index : input_indexes)
  {
    size_t input_rank = _ctx.at(input_index).shape().rank();
    const auto &input_tensor = _tensor_reg->getAclTensor(input_index);
    orig_inputs_acl_tensor_shapes.emplace_back(input_tensor->info()->tensor_shape());
    assert(input_rank == input_tensor->num_dimensions());
    if (input_rank != input_tensor->info()->num_dimensions())
    {
      // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
      input_tensor->info()->set_tensor_shape(acl_common::asTensorShape(
          _ctx.at(input_index).shape(), _current_op_seq_layout, backend_layout, false));
    }
  }

  auto fn = acl_common::generateLayer<arm_compute::CLStackLayer>(inputs, axis, output);

  // Revert disabling applied dim_correction
  assert(inputs.size() == orig_inputs_acl_tensor_shapes.size());
  for (size_t i = 0; i < inputs.size(); ++i)
  {
    inputs.at(i)->info()->set_tensor_shape(orig_inputs_acl_tensor_shapes.at(i));
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Pool2D &node)
{
  auto raw_fn = acl_common::kernelGenPool2D<::arm_compute::CLPoolingLayer>(
      node, _ctx, _tensor_reg, _current_op_seq_layout,
      acl_common::convertPoolType(node.param().op_type));

  const auto ofm_index{node.getOutputs().at(0)};
  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  const auto activation = node.param().activation;
  _return_fn = std::make_unique<exec::FunctionSequence>(
      asAclFunction(std::move(raw_fn)),
      ActivationBuilder::generate(activation, ofm_tensor->handle()));
}

void KernelGenerator::visit(const ir::operation::Permute &node)
{
  const auto ofm_idx{node.getOutputs().at(0)};
  const auto ifm_idx{node.getInputs().at(0)};
  const auto permute_type = node.getPermuteType();
  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_idx).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_idx).get();
  const auto rank = _ctx.at(ofm_idx).shape().rank();
  assert(_ctx.at(ifm_idx).shape().rank() == _ctx.at(ofm_idx).shape().rank());

  std::unique_ptr<::arm_compute::IFunction> fn;
  arm_compute::PermutationVector pv;
  if (permute_type == ir::operation::Permute::Type::NCHW_TO_NHWC && rank == 4)
  {
    // WHCN -> CWHN
    pv = arm_compute::PermutationVector{2, 0, 1};

    fn = acl_common::generateLayer<arm_compute::CLPermute>(ifm_tensor->handle(),
                                                           ofm_tensor->handle(), pv);
  }
  else if (permute_type == ir::operation::Permute::Type::NHWC_TO_NCHW && rank == 4)
  {
    // CWHN -> WHCN
    pv = arm_compute::PermutationVector{1, 2, 0};

    fn = acl_common::generateLayer<::arm_compute::CLPermute>(ifm_tensor->handle(),
                                                             ofm_tensor->handle(), pv);
  }
  else
  {
    fn = acl_common::generateLayer<arm_compute::CLCopy>(ifm_tensor->handle(), ofm_tensor->handle());
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ResizeBilinear &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ResizeBilinear::Input::INPUT)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLScale>(
      ifm_tensor->handle(), ofm_tensor->handle(), ::arm_compute::InterpolationPolicy::BILINEAR,
      ::arm_compute::BorderMode::REPLICATE, ::arm_compute::PixelValue(0.f),
      ::arm_compute::SamplingPolicy::TOP_LEFT);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ResizeNearestNeighbor &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ResizeNearestNeighbor::Input::INPUT)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLScale>(
      ifm_tensor->handle(), ofm_tensor->handle(),
      ::arm_compute::InterpolationPolicy::NEAREST_NEIGHBOR, ::arm_compute::BorderMode::REPLICATE,
      ::arm_compute::PixelValue(0.f), ::arm_compute::SamplingPolicy::TOP_LEFT);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::RNN &node)
{
  const auto output_index{node.getOutputs().at(ir::operation::RNN::Output::OUTPUT)};
  const auto hidden_state_out_index{
      node.getOutputs().at(ir::operation::RNN::Output::HIDDEN_STATE_OUT)};

  const auto input_index{node.getInputs().at(ir::operation::RNN::Input::INPUT)};
  const auto weights_index{node.getInputs().at(ir::operation::RNN::Input::WEIGHTS)};
  const auto recurrent_weights_index{
      node.getInputs().at(ir::operation::RNN::Input::RECURRENT_WEIGHTS)};
  const auto bias_index{node.getInputs().at(ir::operation::RNN::Input::BIAS)};
  const auto hidden_state_in_index{node.getInputs().at(ir::operation::RNN::Input::HIDDEN_STATE_IN)};

  const auto activation = node.param().activation;

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto hidden_state_out_tensor = _tensor_reg->getAclTensor(hidden_state_out_index).get();

  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();
  auto weights_tensor = _tensor_reg->getAclTensor(weights_index).get();
  auto recurrent_weights_tensor = _tensor_reg->getAclTensor(recurrent_weights_index).get();
  auto bias_tensor = _tensor_reg->getAclTensor(bias_index).get();
  auto hidden_state_in_tensor = _tensor_reg->getAclTensor(hidden_state_in_index).get();
  auto act_info = ::onert::backend::acl_common::asActivationLayerInfo(activation);

  auto copy_layer = acl_common::generateLayer<arm_compute::CLCopy>(
      hidden_state_in_tensor->handle(), hidden_state_out_tensor->handle());
  _return_fn = asAclFunction(std::move(copy_layer));

  auto fn = acl_common::generateLayer<arm_compute::CLRNNLayer>(
      _tensor_builder->acl_tensor_manager()->internal_buffer_manager(), input_tensor->handle(),
      weights_tensor->handle(), recurrent_weights_tensor->handle(), bias_tensor->handle(),
      hidden_state_out_tensor->handle(), output_tensor->handle(), act_info);
  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::SpaceToBatchND &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::SpaceToBatchND::Input::INPUT)};
  const auto block_size_index{
      node.getInputs().at(ir::operation::SpaceToBatchND::Input::BLOCK_SIZE)};
  const auto paddings_index{node.getInputs().at(ir::operation::SpaceToBatchND::Input::PADDINGS)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto block_size_tensor = _tensor_reg->getAclTensor(block_size_index).get();
  auto paddings_tensor = _tensor_reg->getAclTensor(paddings_index).get();

  assert(_ctx.at(block_size_index).data());
  assert(_ctx.at(paddings_index).data());

  auto fn = acl_common::generateLayer<arm_compute::CLSpaceToBatchLayer>(
      ifm_tensor->handle(), block_size_tensor->handle(), paddings_tensor->handle(),
      ofm_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::SpaceToDepth &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::SpaceToDepth::Input::INPUT)};

  auto block_size = node.param().block_size;

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLSpaceToDepthLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), block_size);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::EmbeddingLookup &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto lookups_index{node.getInputs().at(ir::operation::EmbeddingLookup::Input::LOOKUPS)};
  const auto values_index{node.getInputs().at(ir::operation::EmbeddingLookup::Input::VALUES)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto lookups_tensor = _tensor_reg->getAclTensor(lookups_index).get();
  auto values_tensor = _tensor_reg->getAclTensor(values_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLEmbeddingLookup>(
      values_tensor->handle(), output_tensor->handle(), lookups_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::L2Normalization &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::L2Normalization::Input::INPUT)};

  // {CL|Neon}L2Normalization performs the reduction only along dimension 0
  // L2 Normalization always performs the reduction along the depth axis
  // Thus, we repurpose {CL|Neon}NormalizationLayers to act as depthwise L2 normalizations by
  // choosing normalization parameters as below

  const auto &ifm_shape = _ctx.at(ifm_index).shape();
  // TODO Support optional constant dimension that normalization would be performed on
  const auto normalization_axis = _ctx.at(ifm_index).shape().rank() - 1;
  int32_t radius =
      2 * ifm_shape.dim(normalization_axis) + 1; // normSize = depth(last dimension) * 2 + 1
  float alpha = 1.0f;                            // In the implementation to make alpha_ become 1
  float beta = 0.5f;                             // pow(reduction, -0.5) = 1 / sqrt(reduction)
  float bias = 0.0f;                             // Don't offset the reduction.

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  const auto norm_info = ::arm_compute::NormalizationLayerInfo(::arm_compute::NormType::CROSS_MAP,
                                                               radius, alpha, beta, bias, false);

  auto fn = acl_common::generateLayer<arm_compute::CLNormalizationLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), norm_info);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::HashtableLookup &node)
{
  const auto output_index{node.getOutputs().at(ir::operation::HashtableLookup::Output::OUTPUT)};
  const auto hits_index{node.getOutputs().at(ir::operation::HashtableLookup::Output::HITS)};

  const auto lookups_index{node.getInputs().at(ir::operation::HashtableLookup::Input::LOOKUPS)};
  const auto keys_index{node.getInputs().at(ir::operation::HashtableLookup::Input::KEYS)};
  const auto values_index{node.getInputs().at(ir::operation::HashtableLookup::Input::VALUES)};

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto hits_tensor = _tensor_reg->getAclTensor(hits_index).get();

  auto lookups_tensor = _tensor_reg->getAclTensor(lookups_index).get();
  auto keys_tensor = _tensor_reg->getAclTensor(keys_index).get();
  auto values_tensor = _tensor_reg->getAclTensor(values_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLHashtableLookup>(
      lookups_tensor->handle(), keys_tensor->handle(), values_tensor->handle(),
      output_tensor->handle(), hits_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::PReLU &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::PReLU::Input::INPUT)};
  const auto alpha_index{node.getInputs().at(ir::operation::PReLU::Input::ALPHA)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto alpha_tensor = _tensor_reg->getAclTensor(alpha_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLPReluLayer>(
      ifm_tensor->handle(), alpha_tensor->handle(), ofm_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::TransposeConv &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ker_index{node.getInputs().at(ir::operation::TransposeConv::Input::KERNEL)};
  const auto ifm_index{node.getInputs().at(ir::operation::TransposeConv::Input::INPUT)};

  const auto ofm_shape = _ctx.at(ofm_index).shape().asFeature(_current_op_seq_layout);
  const auto ifm_shape = _ctx.at(ifm_index).shape().asFeature(_current_op_seq_layout);
  const auto ker_shape = _ctx.at(ker_index).shape().asFeature(_current_op_seq_layout);

  const auto stride = node.param().stride;

  assert((node.param().padding.type == ir::PaddingType::SAME) ||
         (node.param().padding.type == ir::PaddingType::VALID));
  auto padding = ir::calculatePadding(node.param().padding, ofm_shape, ifm_shape, stride,
                                      ker_shape.W, ker_shape.H);
  uint32_t invalid_horizontal = 0;
  uint32_t invalid_vertical = 0;
  if (node.param().padding.type == ir::PaddingType::VALID)
  {
    invalid_horizontal =
        ofm_shape.W - (1 + (ifm_shape.W - 1) * stride.horizontal) - (ker_shape.W - 1);
    invalid_vertical = ofm_shape.H - (1 + (ifm_shape.H - 1) * stride.vertical) - (ker_shape.H - 1);
  }

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto ker_tensor = _tensor_reg->getAclTensor(ker_index).get();

  const auto tconv_info = acl_common::asPadStrideInfo(padding, stride);

  auto fn = acl_common::generateLayer<arm_compute::CLTransposeConvLayer>(
      _tensor_builder->acl_tensor_manager()->internal_buffer_manager(), ifm_tensor->handle(),
      ker_tensor->handle(), nullptr, ofm_tensor->handle(), tconv_info, invalid_horizontal,
      invalid_vertical);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::SquaredDifference &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto lhs_index{node.getInputs().at(ir::operation::SquaredDifference::Input::LHS)};
  const auto rhs_index{node.getInputs().at(ir::operation::SquaredDifference::Input::RHS)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto lhs_tensor = _tensor_reg->getAclTensor(lhs_index).get();
  auto rhs_tensor = _tensor_reg->getAclTensor(rhs_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLElementwiseSquaredDiff>(
      lhs_tensor->handle(), rhs_tensor->handle(), ofm_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::TopKV2 &node)
{
  const auto outputValues_index{node.getOutputs().at(ir::operation::TopKV2::Output::OUTPUT_VALUES)};
  const auto outputIndices_index{
      node.getOutputs().at(ir::operation::TopKV2::Output::OUTPUT_INDICES)};

  const auto inputData_index{node.getInputs().at(ir::operation::TopKV2::Input::INPUT)};

  // Currently, we only support the vector input.
  assert(_ctx.at(inputData_index).shape().rank() == 1 ||
         _ctx.at(inputData_index).shape().rank() == 2);

  const auto k = node.param().k;

  auto values_tensor = _tensor_reg->getAclTensor(outputValues_index).get();
  auto indices_tensor = _tensor_reg->getAclTensor(outputIndices_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(inputData_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLTopKV2>(
      input_tensor->handle(), k, values_tensor->handle(), indices_tensor->handle());

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Gather &node)
{
  const auto ofm_index{node.getOutputs().at(0)};

  const auto ifm_index{node.getInputs().at(ir::operation::Gather::Input::INPUT)};
  const auto indices_index{node.getInputs().at(ir::operation::Gather::Input::INDICES)};

  const auto ifm_rank = _ctx.at(ifm_index).shape().rank();
  const auto axis_raw = node.param().axis;
  const auto axis_value = (axis_raw < 0 ? (ifm_rank + axis_raw) : axis_raw);
  const int axis = ::onert::backend::acl_common::ToARMComputeAxis(ifm_rank, axis_value).value();

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto indices_tensor = _tensor_reg->getAclTensor(indices_index).get();

  // NOTE The frontend layout and backend layout must be the same for this operation.
  //      If not the same, we have to add a stage(?) to perform permutation of output tensor. It
  //      is not not efficient even if it works well. If so, it would be better to set the
  //      layout of these backend tensors to the same layout.
  //      There is also one thing we have to think about. This operation depends on the layout of
  //      a model. For example, if a model in NHWC has this operation as output rank == 4, indices
  //      rank == 2 and axis == 2, this operation should work as the axis W and C, but the axis W
  //      and C are not sequential in NCHW. So the backend in NCHW cannot handle this case.
  const auto backend_layout = ofm_tensor->layout();
  UNUSED_RELEASE(backend_layout);
  assert(backend_layout == ifm_tensor->layout());
  assert(backend_layout == indices_tensor->layout());
  assert(ifm_rank < 4 || _current_op_seq_layout == backend_layout);

  // input is n-D, indices k-D, output is (n + k - 1)-D
  size_t n = ifm_rank;
  assert(n == ifm_tensor->num_dimensions());
  size_t k = _ctx.at(indices_index).shape().rank();
  assert(k == indices_tensor->num_dimensions());

  // Disable applied dim_correction
  const auto orig_ifm_acl_tensor_shape = ifm_tensor->info()->tensor_shape();
  if (n != ifm_tensor->info()->num_dimensions())
  {
    // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
    const auto ifm = _ctx.at(ifm_index);
    ifm_tensor->info()->set_tensor_shape(
        acl_common::asTensorShape(ifm.shape(), _current_op_seq_layout, backend_layout, false));
  }
  const auto orig_indice_acl_tensor_shape = indices_tensor->info()->tensor_shape();
  if (k != indices_tensor->info()->num_dimensions())
  {
    // This means that high dimension's value is 1 and indices tensor is applied dim_correction
    const auto indices = _ctx.at(indices_index);
    indices_tensor->info()->set_tensor_shape(
        acl_common::asTensorShape(indices.shape(), _current_op_seq_layout, backend_layout, false));
  }

  auto fn = acl_common::generateLayer<arm_compute::CLGatherEx>(
      ifm_tensor->handle(), indices_tensor->handle(), ofm_tensor->handle(), axis);

  // Revert disabling applied dim_correction
  ifm_tensor->info()->set_tensor_shape(orig_ifm_acl_tensor_shape);
  indices_tensor->info()->set_tensor_shape(orig_indice_acl_tensor_shape);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ArgMax &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ArgMax::Input::INPUT)};
  const auto axis_index{node.getInputs().at(ir::operation::ArgMax::Input::AXIS)};

  auto ifm_shape = _ctx.at(ifm_index).shape();
  auto ofm_shape = _ctx.at(ofm_index).shape();

  assert((ifm_shape.rank() - 1) == ofm_shape.rank());

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  const auto ifm_rank = _ctx.at(ifm_index).shape().rank();
  auto frontend_layout = _current_op_seq_layout;
  auto backend_layout = ifm_tensor->layout();

  int axis_value = _ctx.at(axis_index).asScalar<int32_t>();
  if (axis_value < 0)
  {
    axis_value += ifm_rank;
  }

  auto acl_axis =
      acl_common::ToARMComputeAxis(ifm_rank, axis_value, frontend_layout, backend_layout).value();

  auto fn = acl_common::generateLayer<arm_compute::CLArgMinMaxLayer>(
      ifm_tensor->handle(), acl_axis, ofm_tensor->handle(),
      ::arm_compute::ReductionOperation::ARG_IDX_MAX);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::LocalResponseNormalization &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{
      node.getInputs().at(ir::operation::LocalResponseNormalization::Input::INPUT)};

  auto radius = node.param().radius;
  auto alpha = node.param().alpha;
  auto beta = node.param().beta;
  auto bias = node.param().bias;

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  const auto norm_info = ::arm_compute::NormalizationLayerInfo(
      ::arm_compute::NormType::CROSS_MAP, radius * 2 + 1, alpha, beta, bias, false);

  auto fn = acl_common::generateLayer<arm_compute::CLNormalizationLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), norm_info);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::DepthToSpace &node)
{
  const auto output_index{node.getOutputs().at(0)};
  const auto input_index{node.getInputs().at(ir::operation::DepthToSpace::Input::INPUT)};

  auto block_size = node.param().block_size;
  assert(block_size > 0);

  auto output_tensor = _tensor_reg->getAclTensor(output_index).get();
  auto input_tensor = _tensor_reg->getAclTensor(input_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLDepthToSpaceLayer>(
      input_tensor->handle(), output_tensor->handle(), block_size);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Split &node)
{
  const auto ifm_index{node.getInputs().at(ir::operation::Split::Input::INPUT)};
  const auto axis_index{node.getInputs().at(ir::operation::Split::Input::AXIS)};

  assert(node.param().num_splits == static_cast<int>(node.getOutputs().size()));

  const auto ifm_rank = _ctx.at(ifm_index).shape().rank();
  std::vector<ir::OperandIndex> output_indexes;
  for (const auto &output : node.getOutputs())
    output_indexes.emplace_back(output);

  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  std::vector<arm_compute::ICLTensor *> output_tensors;
  for (const auto &ofm_ind : output_indexes)
    output_tensors.emplace_back(_tensor_reg->getAclTensor(ofm_ind).get()->handle());

  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = ifm_tensor->layout();
  auto axis = _ctx.at(axis_index).asScalar<int32_t>();
  if (axis < 0)
    axis += ifm_rank;
  axis = acl_common::ToARMComputeAxis(ifm_rank, axis, frontend_layout, backend_layout).value();

  auto fn =
      acl_common::generateLayer<arm_compute::CLSplit>(ifm_tensor->handle(), output_tensors, axis);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::SplitV &node)
{
  const auto ifm_index{node.getInputs().at(ir::operation::SplitV::Input::INPUT)};
  const auto size_split_index{node.getInputs().at(ir::operation::SplitV::Input::SIZE_SPLITS)};
  const auto split_dim_index{node.getInputs().at(ir::operation::SplitV::Input::SPLIT_DIM)};

  assert(node.param().num_splits == static_cast<int>(node.getOutputs().size()));

  const size_t ifm_rank = _ctx.at(ifm_index).shape().rank();
  std::vector<ir::OperandIndex> output_indexes;
  for (const auto &output : node.getOutputs())
    output_indexes.emplace_back(output);

  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();
  auto size_split_tensor = _tensor_reg->getAclTensor(size_split_index).get();

  std::vector<arm_compute::ICLTensor *> output_tensors;
  for (const auto &ofm_ind : output_indexes)
    output_tensors.emplace_back(_tensor_reg->getAclTensor(ofm_ind).get()->handle());

  auto fn = std::make_unique<arm_compute::CLSplitVEx>();
  const auto split_dim_op = _ctx.at(split_dim_index);
  if (split_dim_op.isConstant())
  {
    int32_t split_dim = split_dim_op.asScalar<int32_t>();
    uint32_t split_dim_revised = (split_dim < 0) ? (split_dim + ifm_rank) : split_dim;
    const auto frontend_layout = _current_op_seq_layout;
    const auto backend_layout = ifm_tensor->layout();

    if (ifm_rank != ifm_tensor->info()->num_dimensions())
    {
      // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
      ifm_tensor->info()->set_tensor_shape(acl_common::asTensorShape(
          _ctx.at(ifm_index).shape(), _current_op_seq_layout, backend_layout, false));
    }

    split_dim_revised =
        acl_common::ToARMComputeAxis(ifm_rank, split_dim_revised, frontend_layout, backend_layout)
            .value();
    fn->configure(ifm_tensor->handle(), size_split_tensor->handle(), split_dim_revised,
                  output_tensors, node.param().num_splits);
  }
  else
  {
    throw std::runtime_error("Non-constant split_dim NYI for acl_cl backend");
  }

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Unpack &node)
{
  const auto input_index{node.getInputs().at(ir::operation::Unpack::Input::INPUT)};
  auto axis{node.param().axis};

  const auto input_rank = _ctx.at(input_index).shape().rank();

  std::vector<ir::OperandIndex> output_indexes;
  for (const auto &output_index : node.getOutputs())
    output_indexes.emplace_back(output_index);

  auto input = _tensor_reg->getAclTensor(input_index).get()->handle();
  std::vector<arm_compute::ICLTensor *> outputs;
  for (const auto &output_index : output_indexes)
    outputs.emplace_back(_tensor_reg->getAclTensor(output_index)->handle());

  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = _tensor_reg->getAclTensor(input_index).get()->layout();
  if (axis < 0)
    axis += input_rank;
  axis = acl_common::ToARMComputeAxis(input_rank, axis, frontend_layout, backend_layout).value();

  // Disable applied dim_correction
  std::vector<arm_compute::TensorShape> orig_outputs_acl_tensor_shapes;
  for (const auto &output_index : output_indexes)
  {
    size_t output_rank = _ctx.at(output_index).shape().rank();
    const auto &output_tensor = _tensor_reg->getAclTensor(output_index);
    orig_outputs_acl_tensor_shapes.emplace_back(output_tensor->info()->tensor_shape());
    assert(output_rank == output_tensor->num_dimensions());
    if (output_rank != output_tensor->info()->num_dimensions())
    {
      // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
      output_tensor->info()->set_tensor_shape(acl_common::asTensorShape(
          _ctx.at(output_index).shape(), _current_op_seq_layout, backend_layout, false));
    }
  }

  auto fn = acl_common::generateLayer<arm_compute::CLUnstack>(input, outputs, axis);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::Pad &node)
{
  const auto input_index{node.getInputs().at(ir::operation::Pad::Input::INPUT)};
  const auto pad_index{node.getInputs().at(ir::operation::Pad::Input::PAD)};
  const auto output_index{node.getOutputs().at(0)};
  assert(_ctx.at(pad_index).data());

  auto rank = _ctx.at(input_index).shape().rank();
  auto pad_base = _ctx.at(pad_index).data()->base();

  auto input_type = _ctx.at(input_index).typeInfo();
  auto data_type = acl_common::asDataType(input_type.type());
  auto quant_info = ::arm_compute::QuantizationInfo(input_type.scale(), input_type.offset());
  const auto pixel_value = ::arm_compute::PixelValue(0, data_type, quant_info);

  auto input = _tensor_reg->getAclTensor(input_index).get()->handle();
  auto output = _tensor_reg->getAclTensor(output_index).get()->handle();

  const auto frontend_layout = _current_op_seq_layout;
  const auto backend_layout = _tensor_reg->getAclTensor(input_index).get()->layout();

  ::arm_compute::PaddingList padding_list;
  padding_list.resize(rank);
  for (int32_t n = 0; n < rank; ++n)
  {
    const int32_t *from = reinterpret_cast<const int32_t *>(pad_base) + (n * 2);

    const auto axis =
        acl_common::ToARMComputeAxis(rank, n, frontend_layout, backend_layout).value();
    padding_list[axis] = ::arm_compute::PaddingInfo{from[0], from[1]};
  }

  // Disable applied dim_correction
  size_t input_rank = _ctx.at(input_index).shape().rank();
  const auto &input_tensor = _tensor_reg->getAclTensor(input_index);
  assert(input_rank == input_tensor->num_dimensions());
  if (input_rank != input_tensor->info()->num_dimensions())
  {
    // This means that high dimension's value is 1 and ifm tensor is applied dim_correction
    input_tensor->info()->set_tensor_shape(acl_common::asTensorShape(
        _ctx.at(input_index).shape(), frontend_layout, backend_layout, false));
  }

  auto fn =
      acl_common::generateLayer<arm_compute::CLPadLayer>(input, output, padding_list, pixel_value);

  // Do not revert disabling applied dim_correction CLPadKernel has cl kernel for 4-dimension
  // It would produce a mistach of result

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ConvertFp32ToFp16 &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ConvertFp32ToFp16::Input::INPUT)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLDepthConvertLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), ::arm_compute::ConvertPolicy::SATURATE, 0);

  _return_fn = asAclFunction(std::move(fn));
}

void KernelGenerator::visit(const ir::operation::ConvertFp16ToFp32 &node)
{
  const auto ofm_index{node.getOutputs().at(0)};
  const auto ifm_index{node.getInputs().at(ir::operation::ConvertFp16ToFp32::Input::INPUT)};

  auto ofm_tensor = _tensor_reg->getAclTensor(ofm_index).get();
  auto ifm_tensor = _tensor_reg->getAclTensor(ifm_index).get();

  auto fn = acl_common::generateLayer<arm_compute::CLDepthConvertLayer>(
      ifm_tensor->handle(), ofm_tensor->handle(), ::arm_compute::ConvertPolicy::SATURATE, 0);

  _return_fn = asAclFunction(std::move(fn));
}

} // namespace acl_cl
} // namespace backend
} // namespace onert
