/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved
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

#ifndef __LUCI_LOGEX_CIRCLE_NODE_SUMMARY_BUILDERS__
#define __LUCI_LOGEX_CIRCLE_NODE_SUMMARY_BUILDERS__

#include "CircleNodeSummaryBuilder.h"

#include <luci/IR/CircleNode.h>

#include <string>
#include <vector>

namespace luci
{

class CircleNodeWithXSummaryBuilder : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
};

class CircleNodeWithINPUTSummaryBuilder : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
};

class CircleNodeWithXYSummaryBuilder : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
};

} // namespace luci

namespace luci
{

class CircleAbsSummaryBuilder final : public CircleNodeWithXSummaryBuilder
{
};

class CircleAddSummaryBuilder final : public CircleNodeWithXYSummaryBuilder
{
private:
  bool validate(const luci::CircleNode *node);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleAddNSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *node);
};

class CircleArgMaxSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleArgMinSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleAveragePool2DSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  bool validate(const luci::CircleNode *node);
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleBatchMatMulSummaryBuilder final : public CircleNodeWithXYSummaryBuilder
{
private:
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleBatchToSpaceNDSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
};

class CircleBCQFullyConnectedSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  bool validate(const luci::CircleNode *node);
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleBCQGatherSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleBidirectionalSequenceLSTMSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleBidirectionalSequenceLSTMOutSummaryBuilder final
  : public CircleNodeWithINPUTSummaryBuilder
{
};

class CircleCastSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleCeilSummaryBuilder final : public CircleNodeWithXSummaryBuilder
{
};

class CircleConcatenationSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  bool validate(const luci::CircleNode *node);
  std::vector<std::string> get_input_names(const luci::CircleNode *node);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleConstSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  void update_status(locop::NodeSummary &s);
};

class CircleConv2DSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  bool validate(const luci::CircleNode *node);
  std::vector<std::string> get_input_names(const luci::CircleNode *);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleCosSummaryBuilder final : public CircleNodeWithXSummaryBuilder
{
};

class CircleCustomSummaryBuilder final : public CircleNodeSummaryBuilder
{
private:
  std::vector<std::string> get_input_names(const luci::CircleNode *node);
  void build_attributes(const luci::CircleNode *node, locop::NodeSummary &s);
};

class CircleCustomOutSummaryBuilder final : public CircleNodeWithINPUTSummaryBuilder
{
};

} // namespace luci

#endif // __LUCI_LOGEX_CIRCLE_NODE_SUMMARY_BUILDERS__
