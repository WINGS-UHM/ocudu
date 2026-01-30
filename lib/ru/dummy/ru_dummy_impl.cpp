/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_dummy_impl.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/ru/dummy/ru_dummy_configuration.h"
#include "ocudu/support/error_handling.h"
#include "ocudu/support/math/math_utils.h"
#include "ocudu/support/ocudu_assert.h"
#include <chrono>
#include <cstdint>
#include <thread>

using namespace ocudu;

static uint64_t get_current_system_slot(std::chrono::microseconds slot_duration, uint64_t nof_slots_in_all_hyper_sfns)
{
  // Get the time since the epoch.
  auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());

  return (time_since_epoch / slot_duration) % nof_slots_in_all_hyper_sfns;
}

ru_dummy_impl::ru_dummy_impl(const ru_dummy_configuration& config, ru_dummy_dependencies dependencies) noexcept :
  are_metrics_enabled(config.are_metrics_enabled),
  logger(dependencies.logger),
  executor(*dependencies.executor),
  timing_notifier(dependencies.timing_notifier),
  slot_duration(static_cast<unsigned>(config.time_scaling * 1000.0 / pow2(to_numerology_value(config.scs)))),
  max_processing_delay_slots(config.max_processing_delay_slots),
  current_slot(config.scs, config.max_processing_delay_slots),
  metrics_collector({})
{
  ocudu_assert(config.max_processing_delay_slots > 0, "The maximum processing delay must be greater than 0.");

  sectors.reserve(config.nof_sectors);

  std::vector<ru_dummy_sector*> sector_ptrs;
  sector_ptrs.reserve(config.nof_sectors);

  for (unsigned i_sector = 0; i_sector != config.nof_sectors; ++i_sector) {
    auto& sector = sectors.emplace_back(std::make_unique<ru_dummy_sector>(
        config.dl_processing_delay, logger, dependencies.symbol_notifier, dependencies.error_notifier));
    sector_ptrs.emplace_back(sector.get());
  }

  metrics_collector = ru_dummy_metrics_collector(std::move(sector_ptrs));
}

void ru_dummy_impl::start()
{
  // Get initial system slot.
  uint64_t initial_system_slot = get_current_system_slot(slot_duration, current_slot.nof_slots_in_all_hyper_sfns());
  current_slot                 = slot_point_extended(current_slot.scs(), initial_system_slot);

  stop_control.reset();

  // Start each of the sectors.
  for (auto& sector : sectors) {
    sector->start();
  }

  // Start the loop execution.
  defer_loop();
}

void ru_dummy_impl::stop()
{
  // Stop each of the sectors.
  for (auto& sector : sectors) {
    sector->stop();
  }

  // Signal stop to asynchronous thread.
  // The timing loop must be stopped last as it will clean up all pending requests.
  stop_control.stop();
}

void ru_dummy_impl::defer_loop()
{
  auto token = stop_control.get_token();
  if (OCUDU_UNLIKELY(token.is_stop_requested())) {
    return;
  }

  report_fatal_error_if_not(executor.defer(unique_function<void(), default_unique_task_buffer_size, true>(
                                [this, defer_token = std::move(token)]() noexcept OCUDU_RTSAN_NONBLOCKING { loop(); })),
                            "Failed to execute loop method.");
}

void ru_dummy_impl::loop()
{
  // Get the current system slot from the system time.
  uint64_t slot_count = get_current_system_slot(slot_duration, current_slot.nof_slots_in_all_hyper_sfns());

  // Make sure a minimum time between loop executions without crossing boundaries.
  if (slot_count == current_slot.system_slot()) {
    OCUDU_RTSAN_SCOPED_DISABLER(scoped_disabler);
    std::this_thread::sleep_for(minimum_loop_time);
  }

  // Advance the current slot until it is equal to the slot given by the system time.
  while (slot_count != current_slot.system_slot()) {
    // Increment current slot.
    ++current_slot;

    // Notify new slot boundary.
    timing_notifier.on_tti_boundary(tti_boundary_context{.slot       = current_slot + max_processing_delay_slots,
                                                         .time_point = std::chrono::system_clock::now() +
                                                                       (slot_duration * max_processing_delay_slots)});

    // Notify UL half slot.
    slot_point slot = current_slot.without_hyper_sfn();
    timing_notifier.on_ul_half_slot_boundary(slot);

    // Notify UL full slot.
    timing_notifier.on_ul_full_slot_boundary(slot);

    // Notify the slot boundary in all the sectors.
    for (auto& sector : sectors) {
      sector->new_slot_boundary(slot);
    }
  }

  // Feed back the execution of this task.
  defer_loop();
}
