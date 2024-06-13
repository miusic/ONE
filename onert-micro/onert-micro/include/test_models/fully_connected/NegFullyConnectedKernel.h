/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved
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

#ifndef ONERT_MICRO_TEST_MODELS_NEG_FULLY_CONNECTED_KERNEL_H
#define ONERT_MICRO_TEST_MODELS_NEG_FULLY_CONNECTED_KERNEL_H

#include "TestDataFullyConnectedBase.h"

namespace onert_micro
{
namespace test_model
{

namespace neg_fully_connected_wrong_weight_shape
{
/*
 * FullyConnected Kernel with wrong weight shape (rank should be 2):
 *
 * Input(1, 64)   Weight(1, 8, 64)    Bias(8)
 *            \        |            /
 *             \       |           /
 *               FullyConnected
 *                     |
 *                Output(1, 8)
 */

const unsigned char test_kernel_model_circle[] = {
  0x18, 0x00, 0x00, 0x00, 0x43, 0x49, 0x52, 0x30, 0x00, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x08, 0x00, 0x10, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x38, 0x00, 0x00, 0x00, 0x8c, 0x01, 0x00, 0x00, 0xa8, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x24, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x84, 0xff, 0xff, 0xff, 0x88, 0xff, 0xff, 0xff, 0x8c, 0xff, 0xff, 0xff,
  0x90, 0xff, 0xff, 0xff, 0x94, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x0e, 0x00, 0x18, 0x00, 0x14, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x04, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
  0x68, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x07, 0x00, 0x08, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00,
  0x54, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x8c, 0xff, 0xff, 0xff,
  0x0c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x6f, 0x75, 0x74, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0xb0, 0xff, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x62, 0x69, 0x61, 0x73, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0xd4, 0xff, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
  0x11, 0x00, 0x00, 0x00, 0x4f, 0x4e, 0x45, 0x2d, 0x74, 0x66, 0x6c, 0x69, 0x74, 0x65, 0x32, 0x63,
  0x69, 0x72, 0x63, 0x6c, 0x65, 0x00, 0x00, 0x00};
} // namespace neg_fully_connected_wrong_weight_shape

namespace neg_fully_connected_wrong_bias_shape
{
/*
 * FullyConnected Kernel with wrong bias shape should be equal to output.dim(1):
 *
 * Input(1, 64)   Weight(1, 8, 64)    Bias(15)
 *            \        |            /
 *             \       |           /
 *               FullyConnected
 *                     |
 *                Output(1, 8)
 */
const unsigned char test_kernel_model_circle[] = {
  0x18, 0x00, 0x00, 0x00, 0x43, 0x49, 0x52, 0x30, 0x00, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x08, 0x00, 0x10, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x38, 0x00, 0x00, 0x00, 0x88, 0x01, 0x00, 0x00, 0xa4, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x24, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x84, 0xff, 0xff, 0xff, 0x88, 0xff, 0xff, 0xff, 0x8c, 0xff, 0xff, 0xff,
  0x90, 0xff, 0xff, 0xff, 0x94, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x0e, 0x00, 0x18, 0x00, 0x14, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x04, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
  0x68, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x07, 0x00, 0x08, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00,
  0x54, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x90, 0xff, 0xff, 0xff,
  0x0c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x6f, 0x75, 0x74, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0xb4, 0xff, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x62, 0x69, 0x61, 0x73, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0xd8, 0xff, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x10, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x11, 0x00, 0x00, 0x00,
  0x4f, 0x4e, 0x45, 0x2d, 0x74, 0x66, 0x6c, 0x69, 0x74, 0x65, 0x32, 0x63, 0x69, 0x72, 0x63, 0x6c,
  0x65, 0x00, 0x00, 0x00};
} // namespace neg_fully_connected_wrong_bias_shape

namespace neg_fully_connected_no_zero_points
{
/*
 * FullyConnected Kernel with quantize type and without zero points
 *
 * Input(1, 64)-Int16   Weight(8, 64) - Int16
 *            \           |
 *             \          |
 *               FullyConnected --- Bias (8) - Int32
 *                     |
 *                Output(1, 8) - Int16
 */

const unsigned char test_kernel_model_circle[] = {
  0x18, 0x00, 0x00, 0x00, 0x43, 0x49, 0x52, 0x30, 0x00, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x08, 0x00, 0x10, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x70, 0x04, 0x00, 0x00, 0x80, 0x06, 0x00, 0x00, 0x9c, 0x06, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x5c, 0x04, 0x00, 0x00, 0x54, 0x04, 0x00, 0x00, 0x4c, 0x04, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0xd2, 0xff, 0xff, 0xff, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0xfd, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff,
  0xf9, 0xff, 0xff, 0xff, 0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0xfc, 0xff, 0xfa, 0xff, 0x03, 0x00, 0xff, 0xff,
  0x03, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x02, 0x00, 0xfc, 0xff, 0x08, 0x00,
  0xfd, 0xff, 0x05, 0x00, 0x00, 0x00, 0xfa, 0xff, 0xfc, 0xff, 0xfb, 0xff, 0x04, 0x00, 0xfa, 0xff,
  0xf8, 0xff, 0x01, 0x00, 0xfc, 0xff, 0xfb, 0xff, 0x00, 0x00, 0xfb, 0xff, 0x01, 0x00, 0x04, 0x00,
  0x04, 0x00, 0xfd, 0xff, 0x00, 0x00, 0xfc, 0xff, 0xfe, 0xff, 0x01, 0x00, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xfd, 0xff, 0x04, 0x00, 0xf6, 0xff, 0xfa, 0xff, 0xfb, 0xff, 0x05, 0x00,
  0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xfd, 0xff, 0x04, 0x00, 0x03, 0x00, 0x02, 0x00,
  0xfd, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0xfb, 0xff, 0x04, 0x00,
  0xfd, 0xff, 0x02, 0x00, 0x05, 0x00, 0x04, 0x00, 0xfd, 0xff, 0x04, 0x00, 0xfa, 0xff, 0x03, 0x00,
  0x05, 0x00, 0x00, 0x00, 0xfa, 0xff, 0xfb, 0xff, 0x08, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0xf8, 0xff, 0x05, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 0x00, 0xf7, 0xff,
  0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0xfb, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x04, 0x00, 0xf9, 0xff, 0xfb, 0xff, 0xfd, 0xff, 0xf5, 0xff, 0x07, 0x00, 0x04, 0x00,
  0x01, 0x00, 0xf6, 0xff, 0xfa, 0xff, 0x02, 0x00, 0x07, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00,
  0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0xff, 0xff, 0x02, 0x00, 0x04, 0x00, 0x04, 0x00,
  0x05, 0x00, 0x07, 0x00, 0xf7, 0xff, 0x09, 0x00, 0x00, 0x00, 0xfd, 0xff, 0x06, 0x00, 0x05, 0x00,
  0x00, 0x00, 0xfb, 0xff, 0x01, 0x00, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xfd, 0xff, 0x00, 0x00,
  0x07, 0x00, 0xff, 0xff, 0x00, 0x00, 0x03, 0x00, 0xfd, 0xff, 0xff, 0xff, 0x00, 0x00, 0xf9, 0xff,
  0x06, 0x00, 0x00, 0x00, 0xfb, 0xff, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xff, 0x04, 0x00, 0xfd, 0xff,
  0x02, 0x00, 0xfa, 0xff, 0x01, 0x00, 0xf9, 0xff, 0xfa, 0xff, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xff,
  0xf3, 0xff, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0xff, 0xff, 0xfd, 0xff, 0x04, 0x00, 0x05, 0x00,
  0x03, 0x00, 0x06, 0x00, 0xfb, 0xff, 0xfc, 0xff, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xfe, 0xff,
  0xfe, 0xff, 0x01, 0x00, 0xf9, 0xff, 0x08, 0x00, 0x02, 0x00, 0x01, 0x00, 0x05, 0x00, 0xfd, 0xff,
  0xfe, 0xff, 0xfd, 0xff, 0x03, 0x00, 0xfe, 0xff, 0x02, 0x00, 0xfa, 0xff, 0x04, 0x00, 0x00, 0x00,
  0xfb, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0x04, 0x00, 0xff, 0xff, 0x02, 0x00, 0x0b, 0x00, 0x01, 0x00,
  0x07, 0x00, 0xfe, 0xff, 0x06, 0x00, 0xf2, 0xff, 0x02, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 0x00,
  0xfc, 0xff, 0x0a, 0x00, 0x03, 0x00, 0x02, 0x00, 0xf7, 0xff, 0x00, 0x00, 0xfb, 0xff, 0x01, 0x00,
  0xfb, 0xff, 0x01, 0x00, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00,
  0xfc, 0xff, 0x05, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xf9, 0xff, 0x05, 0x00, 0xff, 0xff,
  0x07, 0x00, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0xf7, 0xff, 0x01, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0xf7, 0xff, 0xff, 0xff, 0x04, 0x00, 0x04, 0x00,
  0x02, 0x00, 0x03, 0x00, 0xfd, 0xff, 0xfa, 0xff, 0xfb, 0xff, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xfd, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xfa, 0xff, 0xf8, 0xff, 0x05, 0x00, 0xff, 0xff,
  0xfb, 0xff, 0x00, 0x00, 0xfe, 0xff, 0x01, 0x00, 0x02, 0x00, 0xfb, 0xff, 0x04, 0x00, 0xff, 0xff,
  0x00, 0x00, 0xf7, 0xff, 0xf0, 0xff, 0x00, 0x00, 0xfb, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
  0x05, 0x00, 0x01, 0x00, 0x04, 0x00, 0xfe, 0xff, 0xfa, 0xff, 0x07, 0x00, 0xfb, 0xff, 0xf6, 0xff,
  0xff, 0xff, 0x07, 0x00, 0xfd, 0xff, 0x02, 0x00, 0x0b, 0x00, 0x04, 0x00, 0xff, 0xff, 0xfb, 0xff,
  0x02, 0x00, 0x03, 0x00, 0xfd, 0xff, 0x02, 0x00, 0x0a, 0x00, 0x05, 0x00, 0xff, 0xff, 0x05, 0x00,
  0x09, 0x00, 0xfc, 0xff, 0xfd, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00,
  0xfd, 0xff, 0x03, 0x00, 0x04, 0x00, 0x01, 0x00, 0x01, 0x00, 0xfd, 0xff, 0x01, 0x00, 0xfe, 0xff,
  0x06, 0x00, 0xff, 0xff, 0x05, 0x00, 0xfe, 0xff, 0xfd, 0xff, 0x03, 0x00, 0x01, 0x00, 0x04, 0x00,
  0x03, 0x00, 0x0b, 0x00, 0xfa, 0xff, 0xfe, 0xff, 0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0xfd, 0xff,
  0xfe, 0xff, 0x02, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xfd, 0xff, 0xff, 0xff,
  0xfc, 0xff, 0x04, 0x00, 0x01, 0x00, 0x01, 0x00, 0xfb, 0xff, 0x02, 0x00, 0x03, 0x00, 0x06, 0x00,
  0xfc, 0xff, 0xfe, 0xff, 0x01, 0x00, 0x07, 0x00, 0xfc, 0xff, 0xfd, 0xff, 0x02, 0x00, 0x05, 0x00,
  0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0xfc, 0xff,
  0x02, 0x00, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xfd, 0xff, 0x00, 0x00, 0xf5, 0xff,
  0x00, 0x00, 0x05, 0x00, 0x03, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00,
  0xfd, 0xff, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0xfa, 0xff, 0xfd, 0xff, 0x05, 0x00, 0x02, 0x00,
  0xf9, 0xff, 0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0xfd, 0xff, 0xfc, 0xff, 0x07, 0x00, 0x03, 0x00,
  0xfd, 0xff, 0xfd, 0xff, 0x09, 0x00, 0x00, 0x00, 0xfb, 0xff, 0x02, 0x00, 0x02, 0x00, 0x07, 0x00,
  0xfb, 0xff, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x03, 0x00, 0x06, 0x00, 0xff, 0xff,
  0x00, 0x00, 0xff, 0xff, 0xfc, 0xff, 0xfe, 0xff, 0x02, 0x00, 0x03, 0x00, 0xf6, 0xff, 0x00, 0x00,
  0x00, 0x00, 0xfd, 0xff, 0x04, 0x00, 0xfe, 0xff, 0xfd, 0xff, 0x03, 0x00, 0xff, 0xff, 0x05, 0x00,
  0x00, 0x00, 0xfe, 0xff, 0xfd, 0xff, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00,
  0x05, 0x00, 0x07, 0x00, 0xf4, 0xff, 0x0a, 0x00, 0x06, 0x00, 0x07, 0x00, 0x02, 0x00, 0x03, 0x00,
  0xfd, 0xff, 0x08, 0x00, 0xfb, 0xff, 0x04, 0x00, 0x07, 0x00, 0x0a, 0x00, 0xfb, 0xff, 0x03, 0x00,
  0x04, 0x00, 0x04, 0x00, 0xfe, 0xff, 0x02, 0x00, 0x00, 0x00, 0xf7, 0xff, 0x02, 0x00, 0x01, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0xfd, 0xff, 0x04, 0x00, 0xfd, 0xff, 0x00, 0x00,
  0xfb, 0xff, 0xf7, 0xff, 0xfa, 0xff, 0x01, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x07, 0x00, 0x05, 0x00,
  0xfe, 0xff, 0xfb, 0xff, 0xf8, 0xff, 0x0b, 0x00, 0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x01, 0x00, 0x09, 0x00, 0x0c, 0x00, 0x05, 0x00, 0x01, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00,
  0xff, 0xff, 0x01, 0x00, 0x07, 0x00, 0x04, 0x00, 0x07, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xfb, 0xff,
  0xfb, 0xff, 0xff, 0xff, 0xfe, 0xff, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00, 0xff, 0xff,
  0x00, 0x00, 0x04, 0x00, 0x8c, 0xff, 0xff, 0xff, 0x90, 0xff, 0xff, 0xff, 0x94, 0xff, 0xff, 0xff,
  0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x18, 0x00, 0x14, 0x00,
  0x10, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x1c, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0c, 0x00,
  0x07, 0x00, 0x08, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00,
  0x9c, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xff, 0xff,
  0x14, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
  0x3c, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0xb8, 0x19, 0x85, 0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x10, 0x00, 0x0f, 0x00,
  0x08, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x62, 0x69, 0x61, 0x73,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x9e, 0xff, 0xff, 0xff,
  0x14, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
  0x40, 0x00, 0x00, 0x00, 0x90, 0xff, 0xff, 0xff, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0xdd, 0x58, 0xa4, 0x3a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x18, 0x00, 0x14, 0x00, 0x13, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
  0x48, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x04, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x10, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x87, 0xb9, 0x39,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x09, 0x11, 0x00, 0x00, 0x00, 0x4f, 0x4e, 0x45, 0x2d, 0x74, 0x66, 0x6c, 0x69,
  0x74, 0x65, 0x32, 0x63, 0x69, 0x72, 0x63, 0x6c, 0x65, 0x00, 0x00, 0x00};
} // namespace neg_fully_connected_no_zero_points

class NegTestDataWrongWeightShapeFullyConnectedKernel : public NegTestDataBase
{
public:
  NegTestDataWrongWeightShapeFullyConnectedKernel()
  {
    _test_kernel_model_circle = neg_fully_connected_wrong_weight_shape::test_kernel_model_circle;
  }

  ~NegTestDataWrongWeightShapeFullyConnectedKernel() override = default;

  const unsigned char *get_model_ptr() override final { return _test_kernel_model_circle; }

protected:
  const unsigned char *_test_kernel_model_circle;
};

class NegTestDataWrongBiasShapeFullyConnectedKernel : public NegTestDataBase
{
public:
  NegTestDataWrongBiasShapeFullyConnectedKernel()
  {
    _test_kernel_model_circle = neg_fully_connected_wrong_bias_shape::test_kernel_model_circle;
  }

  ~NegTestDataWrongBiasShapeFullyConnectedKernel() override = default;

  const unsigned char *get_model_ptr() override final { return _test_kernel_model_circle; }

protected:
  const unsigned char *_test_kernel_model_circle;
};

class NegTestDataNoZeroPointsFullyConnectedKernel : public NegTestDataBase
{
public:
  NegTestDataNoZeroPointsFullyConnectedKernel()
  {
    _test_kernel_model_circle = neg_fully_connected_no_zero_points::test_kernel_model_circle;
  }

  ~NegTestDataNoZeroPointsFullyConnectedKernel() override = default;

  const unsigned char *get_model_ptr() override final { return _test_kernel_model_circle; }

protected:
  const unsigned char *_test_kernel_model_circle;
};

} // namespace test_model
} // namespace onert_micro

#endif // ONERT_MICRO_TEST_MODELS_NEG_FULLY_CONNECTED_KERNEL_H
