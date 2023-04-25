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

#include <tensorflow/lite/kernels/internal/reference/sub.h>

namespace luci_interpreter
{

void configure_kernel_CircleSub(const circle::Operator *cur_op, BaseRuntimeGraph *runtime_graph)
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

void execute_kernel_CircleSub(const circle::Operator *cur_op, BaseRuntimeGraph *runtime_graph,
                              bool is_inplace)
{
  kernels::TISOKernel kernel(cur_op, runtime_graph);

  const auto *options = cur_op->builtin_options_as_SubOptions();

  tflite::RuntimeShape input_shape1 =
    kernels::getTensorRuntimeShape(kernel.input1(), runtime_graph);
  tflite::RuntimeShape input_shape2 =
    kernels::getTensorRuntimeShape(kernel.input2(), runtime_graph);

  switch (Tensor::element_type(kernel.input1()))
  {
#ifndef DIS_FLOAT
    case DataType::FLOAT32:
    {
      auto tiso_func = tflite::reference_ops::Sub<float>;

      auto broadcast_tiso_func =
        [](const tflite::ArithmeticParams &params, const tflite::RuntimeShape &input1_shape,
           const float *input1_data, const tflite::RuntimeShape &input2_shape,
           const float *input2_data, const tflite::RuntimeShape &output_shape, float *output_data) {
          tflite::reference_ops::BroadcastSubSlow(params, input1_shape, input1_data, input2_shape,
                                                  input2_data, output_shape, output_data);
        };

      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<float>(tiso_func, broadcast_tiso_func, &kernel, options,
                                              std::move(input_shape1), std::move(input_shape2));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<float>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                       options, std::move(input_shape1), std::move(input_shape2));
      }
    }
    break;
#endif // DIS_FLOAT
    case DataType::S64:
    {
      auto tiso_func = tflite::reference_ops::Sub<int64_t>;

      auto broadcast_tiso_func =
        [](const tflite::ArithmeticParams &params, const tflite::RuntimeShape &input1_shape,
           const int64_t *input1_data, const tflite::RuntimeShape &input2_shape,
           const int64_t *input2_data, const tflite::RuntimeShape &output_shape,
           int64_t *output_data) {
          tflite::reference_ops::BroadcastSubSlow(params, input1_shape, input1_data, input2_shape,
                                                  input2_data, output_shape, output_data);
        };

      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<int64_t>(tiso_func, broadcast_tiso_func, &kernel, options,
                                                std::move(input_shape1), std::move(input_shape2));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<int64_t>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                         options, std::move(input_shape1), std::move(input_shape2));
      }
    }
    break;
    case DataType::S32:
    {
      auto tiso_func = tflite::reference_ops::Sub<int32_t>;

      auto broadcast_tiso_func =
        [](const tflite::ArithmeticParams &params, const tflite::RuntimeShape &input1_shape,
           const int32_t *input1_data, const tflite::RuntimeShape &input2_shape,
           const int32_t *input2_data, const tflite::RuntimeShape &output_shape,
           int32_t *output_data) {
          tflite::reference_ops::BroadcastSubSlow(params, input1_shape, input1_data, input2_shape,
                                                  input2_data, output_shape, output_data);
        };

      if (is_inplace)
      {
        kernels::evalTISOInplaceKernel<int32_t>(tiso_func, broadcast_tiso_func, &kernel, options,
                                                std::move(input_shape1), std::move(input_shape2));
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOKernel<int32_t>(tiso_func, broadcast_tiso_func, &kernel, &kernel_data,
                                         options, std::move(input_shape1), std::move(input_shape2));
      }
    }
    break;
#ifndef DIS_QUANT
    case DataType::U8:
    {
      auto tiso_func = [](const tflite::ArithmeticParams &params,
                          const tflite::RuntimeShape &input1_shape, const uint8_t *input1_data,
                          const tflite::RuntimeShape &input2_shape, const uint8_t *input2_data,
                          const tflite::RuntimeShape &output_shape, uint8_t *output_data) {
        tflite::reference_ops::Sub(params, input1_shape, input1_data, input2_shape, input2_data,
                                   output_shape, output_data);
      };
      auto broadcast_tiso_func =
        [](const tflite::ArithmeticParams &params, const tflite::RuntimeShape &input1_shape,
           const uint8_t *input1_data, const tflite::RuntimeShape &input2_shape,
           const uint8_t *input2_data, const tflite::RuntimeShape &output_shape,
           uint8_t *output_data) {
          tflite::reference_ops::BroadcastSubSlow(params, input1_shape, input1_data, input2_shape,
                                                  input2_data, output_shape, output_data);
        };
      if (is_inplace)
      {
        kernels::evalTISOInplaceQuantizedKernel<uint8_t>(tiso_func, broadcast_tiso_func, &kernel,
                                                         options);
      }
      else
      {
        kernels::TISOData kernel_data = kernel.readData();
        kernels::evalTISOQuantizedKernel<uint8_t>(tiso_func, broadcast_tiso_func, &kernel,
                                                  &kernel_data, options);
      }
    }
    break;
#endif // DIS_QUANT
    default:
      assert(false && "Unsupported type.");
  }
}

} // namespace luci_interpreter
