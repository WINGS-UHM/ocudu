/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/ru/dummy/ru_dummy_executor_mapper.h"

using namespace ocudu;

namespace {

/// Implements an RU dummy executor mapper.
class ru_dummy_executor_mapper_impl : public ru_dummy_executor_mapper
{
public:
  /// Contructs the executor mapper with a single executor reference.
  ru_dummy_executor_mapper_impl(task_executor& executor_) : executor(executor_) {}

  // See interface for documentation.
  task_executor& common_executor() override { return executor; }

private:
  // Actual executor.
  task_executor& executor;
};

} // namespace

std::unique_ptr<ru_dummy_executor_mapper> ocudu::create_ru_dummy_executor_mapper(ocudu::task_executor& executor)
{
  return std::make_unique<ru_dummy_executor_mapper_impl>(executor);
}
