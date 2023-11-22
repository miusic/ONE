/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Builders.h"
#include "kernels/Utils.h"

#include "kernels/BinaryOpCommon.h"

#include "PALMul.h"

namespace luci_interpreter
{

namespace
{

#ifndef DIS_QUANT
void evalQuantized(const circle::Tensor *input1, const circle::Tensor *input2,
                   const circle::Tensor *output, const circle::MulOptions *options,
                   BaseRuntimeGraph *runtime_graph, DataType type)
{
  assert(type == DataType::S16 or type == DataType::S8 && "Wrong Type");

  luci_interpreter_pal::ArithmeticParams params{};
  luci_interpreter::RuntimeShape input_shape1 =
    kernels::getTensorRuntimeShape(input1, runtime_graph);
  luci_interpreter::RuntimeShape input_shape2 =
    kernels::getTensorRuntimeShape(input2, runtime_graph);

  const bool need_broadcast =
    luci_interpreter_pal::ProcessBroadcastShapes(input_shape1, input_shape2, &params);

  assert(need_broadcast == false && "Broadcast for INT8 and INT16 not supported now");

  params.input1_offset = -Tensor::zero_point(input1);
  params.input2_offset = -Tensor::zero_point(input2);
  params.output_offset = Tensor::zero_point(output);

  const auto input1_scale = static_cast<double>(Tensor::scale(input1));
  const auto input2_scale = static_cast<double>(Tensor::scale(input2));
  const auto output_scale = static_cast<double>(Tensor::scale(output));

  double real_multiplier = input1_scale * input2_scale / output_scale;

  kernels::quantizeMultiplier(real_multiplier, &params.output_multiplier, &params.output_shift);

  kernels::calculateActivationRangeQuantized(luci_actfunc(options->fused_activation_function()),
                                             output, &params.quantized_activation_min,
                                             &params.quantized_activation_max);
  if (type == DataType::S8)
  {
    luci_interpreter_pal::Mul(
      params, input_shape1.flatSize(),
      kernels::getTensorData<int8_t>(runtime_graph->getDataByTensor(input1)),
      kernels::getTensorData<int8_t>(runtime_graph->getDataByTensor(input2)),
      kernels::getTensorData<int8_t>(runtime_graph->getDataByTensor(output)));
  }
  else
  {
    luci_interpreter_pal::Mul(
      params, input_shape1.flatSize(),
      kernels::getTensorData<int16_t>(runtime_graph->getDataByTensor(input1)),
      kernels::getTensorData<int16_t>(runtime_graph->getDataByTensor(input2)),
      kernels::getTensorData<int16_t>(runtime_graph->getDataByTensor(output)));
  }
}
#endif // DIS_QUANT

} // namespace

void configure_kernel_CircleMul(const circle::Operator *cur_op, BaseRuntimeGraph *runtime_graph)
{
  kernels::TISOKernel kernel(cur_op, runtime_graph);

  LUCI_INTERPRETER_CHECK(Tensor::element_type(kernel.input1()) ==
                         Tensor::element_type(kernel.input2()));
  LUCI_INTERPRETER_CHECK(Tensor::element_type(kernel.input1()) ==
                         Tensor::element_type(kernel.input2()));
#ifndef DIS_QUANT
  if (Tensor::element_type(kernel.input1()) == DataType::S16)
  {
    LUCI_INTERPRETER_CHECK(Tensor::zero_points(kernel.input1()).size() == 1 &&
                           Tensor::zero_points(kernel.input2()).size() == 1);
    LUCI_INTERPRETER_CHECK(Tensor::zero_point(kernel.input1()) == 0 &&
                           Tensor::zero_point(kernel.input2()) == 0 &&
                           Tensor::zero_point(kernel.output()) == 0);
  }
#endif // DIS_QUANT
}

void execute_kernel_CircleMul(const circle::Operator *cur_op, BaseRuntimeGraph *runtime_graph)
{
  kernels::TISOKernel kernel(cur_op, runtime_graph);

  const auto *options = cur_op->builtin_options_as_MulOptions();

  luci_interpreter::RuntimeShape input_shape1 =
    kernels::getTensorRuntimeShape(kernel.input1(), runtime_graph);
  luci_interpreter::RuntimeShape input_shape2 =
    kernels::getTensorRuntimeShape(kernel.input2(), runtime_graph);

  luci_interpreter::RuntimeShape output_shape =
    kernels::getTensorRuntimeShape(kernel.output(), runtime_graph);

  bool is_inplace = runtime_graph->is_inplace_op(cur_op);
  const auto type = Tensor::element_type(kernel.input1());
  switch (type)
  {
#ifndef DIS_FLOAT
    case DataType::FLOAT32:
    {
      auto tiso_func = luci_interpreter_pal::Mul<float>;
      auto broadcast_tiso_func = luci_interpreter_pal::BroadcastMul4DSlow<float>;
      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<float>(tiso_func, broadcast_tiso_func, &kernel, options,
                                              std::move(input_shape1), std::move(input_shape2),
                                              std::move(output_shape));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<float>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                       options, std::move(input_shape1), std::move(input_shape2),
                                       std::move(output_shape));
      }
    }
    break;
#endif // DIS_FLOAT
    case DataType::S64:
    {
      auto tiso_func = luci_interpreter_pal::Mul<int64_t>;
      auto broadcast_tiso_func = luci_interpreter_pal::BroadcastMul4DSlow<int64_t>;
      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<int64_t>(tiso_func, broadcast_tiso_func, &kernel, options,
                                                std::move(input_shape1), std::move(input_shape2),
                                                std::move(output_shape));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<int64_t>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                         options, std::move(input_shape1), std::move(input_shape2),
                                         std::move(output_shape));
      }
    }
    break;
    case DataType::S32:
    {
      auto tiso_func = luci_interpreter_pal::Mul<int32_t>;
      auto broadcast_tiso_func = luci_interpreter_pal::BroadcastMul4DSlow<int32_t>;
      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<int32_t>(tiso_func, broadcast_tiso_func, &kernel, options,
                                                std::move(input_shape1), std::move(input_shape2),
                                                std::move(output_shape));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<int32_t>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                         options, std::move(input_shape1), std::move(input_shape2),
                                         std::move(output_shape));
      }
    }
    break;
#ifndef DIS_QUANT
    case DataType::S8:
    case DataType::S16:
    {
      evalQuantized(kernel.input1(), kernel.input2(), kernel.output(), options, runtime_graph,
                    type);
    }
    break;
#endif // DIS_QUANT
    default:
      assert(false && "Unsupported type.");
  }
}

} // namespace luci_interpreter
