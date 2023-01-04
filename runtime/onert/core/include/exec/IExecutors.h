/*
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#ifndef __ONERT_EXEC_I_EXECUTORS_H__
#define __ONERT_EXEC_I_EXECUTORS_H__

#include "IExecutor.h"

namespace onert
{
namespace exec
{

/**
 * @brief Class to gather executors
 */
class IExecutors
{
public:
  /**
   * @brief Virtual IExecutors destructor
   * @note  Require derived class destructor
   */
  virtual ~IExecutors() = default;

public:
  // TODO Use Executor index
  virtual void emplace(const ir::ModelIndex &model_index, const ir::SubgraphIndex &subg_index,
                       std::unique_ptr<IExecutor> exec) = 0;

  virtual IExecutor *at(const ir::ModelIndex &model_index,
                        const ir::SubgraphIndex &subg_index) const = 0;

  IExecutor *entryExecutor() const { return at(ir::ModelIndex{0}, ir::SubgraphIndex{0}); }

  virtual uint32_t inputSize() const = 0;

  virtual uint32_t outputSize() const = 0;

  virtual const ir::OperandInfo inputInfo(const ir::IOIndex &index) = 0;

  virtual const ir::OperandInfo outputInfo(const ir::IOIndex &index) = 0;

  virtual void execute(const IODescription &desc) = 0;
};

} // namespace exec
} // namespace onert

#endif // __ONERT_EXEC_I_EXECUTORS_H__
