/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "kernels/FullyConnected.h"

#include "kernels/Utils.h"

#include <tensorflow/lite/kernels/internal/reference/fully_connected.h>

#include <stdexcept>

namespace luci_interpreter
{

namespace kernels
{

FullyConnected::FullyConnected(const Tensor *input, const Tensor *weights, const Tensor *bias,
                               Tensor *output, const FullyConnectedParams &params)
    : KernelWithParams<FullyConnectedParams>({input, weights, bias}, {output}, params)
{
}

void FullyConnected::configure()
{
  if (weights()->element_type() != DataType::FLOAT32)
    throw std::runtime_error("Unsupported type.");

  assert(input()->element_type() == DataType::FLOAT32);
  assert(weights()->element_type() == DataType::FLOAT32);
  assert(bias() == nullptr || bias()->element_type() == DataType::FLOAT32);

  const Shape &input_shape = input()->shape();
  const Shape &weights_shape = weights()->shape();

  assert(weights_shape.num_dims() == 2);
  assert(bias() == nullptr || bias()->shape().num_elements() == weights_shape.dim(0));

  assert(input_shape.num_elements() % weights_shape.dim(1) == 0);
  const int32_t batch_size = input_shape.num_elements() / weights_shape.dim(1);
  const int32_t num_units = weights_shape.dim(0);

  output()->resize({batch_size, num_units});
}

void FullyConnected::execute() const { evalFloat(); }

void FullyConnected::evalFloat() const
{
  float activation_min{};
  float activation_max{};
  calculateActivationRange(_params.activation, &activation_min, &activation_max);

  tflite::FullyConnectedParams params{};
  params.float_activation_min = activation_min;
  params.float_activation_max = activation_max;
  params.weights_format = tflite::FullyConnectedWeightsFormat::kDefault;

  tflite::reference_ops::FullyConnected(
      params, getTensorShape(input()), getTensorData<float>(input()), getTensorShape(weights()),
      getTensorData<float>(weights()), getTensorShape(bias()), getTensorData<float>(bias()),
      getTensorShape(output()), getTensorData<float>(output()));
}

} // namespace kernels
} // namespace luci_interpreter
