/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved
 * Copyright 2020 The TensorFlow Authors. All Rights Reserved.
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

#ifndef ONERT_MICRO_PAL_TANH_COMMON_H
#define ONERT_MICRO_PAL_TANH_COMMON_H

#include "PALUnaryOpCommon.h"
#include <cmath>

namespace onert_micro
{
namespace execute
{
namespace pal
{
template <typename T>
inline OMStatus Tanh(const core::OMRuntimeShape &input_shape, const T *input_data,
                     const core::OMRuntimeShape &output_shape, T *output_data)
{
  const uint32_t flat_size = input_shape.flatSize();

  if (flat_size == -1)
    return UnknownError;

  assert(input_data != nullptr);
  assert(output_data != nullptr);

  assert(input_shape == output_shape);

  for (int i = 0; i < flat_size; i++)
  {
    output_data[i] = std::tanh(input_data[i]);
  }

  return Ok;
}
} // namespace pal
} // namespace execute
} // namespace onert_micro

#endif // ONERT_MICRO_PAL_TANH_COMMON_H
