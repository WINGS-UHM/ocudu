/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mac_fapi_p7_sector_fastpath_adaptor_factory_impl.h"
#include "mac_fapi_p7_sector_fastpath_adaptor_impl.h"
#include "mac_fapi_p7_sector_fastpath_adaptor_impl_config.h"
#include "ocudu/fapi/decorator.h"
#include "ocudu/fapi/decorator_factory.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;
using namespace fapi_adaptor;

namespace {

/// MAC-FAPI adaptor wrapper that wraps an adaptor and a FAPI decorator.
class mac_fapi_adaptor_wrapper : public mac_fapi_p7_sector_fastpath_adaptor, public operation_controller
{
public:
  mac_fapi_adaptor_wrapper(std::unique_ptr<mac_fapi_p7_sector_fastpath_adaptor> adaptor_,
                           std::unique_ptr<fapi::fapi_decorator>                decorator_) :
    decorator(std::move(decorator_)), adaptor(std::move(adaptor_))
  {
    ocudu_assert(adaptor, "Invalid MAC-FAPI sector adaptor");

    if (decorator) {
      decorator->set_p7_indications_notifier(adaptor->get_p7_indications_notifier());
      decorator->set_error_indication_notifier(adaptor->get_error_indication_notifier());
      decorator->set_p7_slot_indication_notifier(adaptor->get_p7_slot_indication_notifier());
    }
  }

  // See interface for documentation.
  operation_controller& get_operation_controller() override { return *this; }

  // See interface for documentation.
  void start() override { adaptor->get_operation_controller().start(); }

  // See interface for documentation.
  void stop() override { adaptor->get_operation_controller().stop(); }

  // See interface for documentation.
  fapi::p7_slot_indication_notifier& get_p7_slot_indication_notifier() override
  {
    return decorator ? decorator->get_p7_slot_indication_notifier() : adaptor->get_p7_slot_indication_notifier();
  }

  // See interface for documentation.
  fapi::error_indication_notifier& get_error_indication_notifier() override
  {
    return decorator ? decorator->get_error_indication_notifier() : adaptor->get_error_indication_notifier();
  }

  // See interface for documentation.
  fapi::p7_indications_notifier& get_p7_indications_notifier() override
  {
    return decorator ? decorator->get_p7_indications_notifier() : adaptor->get_p7_indications_notifier();
  }

  // See interface for documentation.
  mac_cell_result_notifier& get_cell_result_notifier() override { return adaptor->get_cell_result_notifier(); }

  // See interface for documentation.
  void set_cell_slot_handler(mac_cell_slot_handler& mac_slot_handler) override
  {
    adaptor->set_cell_slot_handler(mac_slot_handler);
  }

  // See interface for documentation.
  void set_cell_rach_handler(mac_cell_rach_handler& mac_rach_handler) override
  {
    adaptor->set_cell_rach_handler(mac_rach_handler);
  }

  // See interface for documentation.
  void set_cell_pdu_handler(mac_pdu_handler& handler) override { adaptor->set_cell_pdu_handler(handler); }

  // See interface for documentation.
  void set_cell_crc_handler(mac_cell_control_information_handler& handler) override
  {
    adaptor->set_cell_crc_handler(handler);
  }

private:
  std::unique_ptr<fapi::fapi_decorator>                decorator;
  std::unique_ptr<mac_fapi_p7_sector_fastpath_adaptor> adaptor;
};

} // namespace

std::unique_ptr<mac_fapi_p7_sector_fastpath_adaptor> ocudu::fapi_adaptor::create_mac_fapi_p7_sector_fastpath_adaptor(
    const mac_fapi_p7_sector_fastpath_adaptor_config&     config,
    mac_fapi_p7_sector_fastpath_adaptor_impl_dependencies dependencies)
{
  auto& base_dependencies = dependencies.base_dependencies;

  // Create FAPI decorators configuration.
  fapi::decorator_config decorator_cfg;
  if (config.log_level == ocudulog::basic_levels::debug) {
    decorator_cfg.logging_cfg.emplace(
        fapi::logging_decorator_config{.sector_id            = config.sector_id,
                                       .logger               = ocudulog::fetch_basic_logger("FAPI", true),
                                       .p7_gateway           = base_dependencies.p7_gateway,
                                       .p7_last_req_notifier = base_dependencies.p7_last_req_notifier});
  }
  if (config.l2_nof_slots_ahead != 0) {
    ocudu_assert(base_dependencies.bufferer_task_executor, "Invalid executor for the FAPI message bufferer decorator");
    decorator_cfg.bufferer_cfg.emplace(
        fapi::message_bufferer_decorator_config{.sector_id            = config.sector_id,
                                                .l2_nof_slots_ahead   = config.l2_nof_slots_ahead,
                                                .scs                  = config.scs,
                                                .executor             = *base_dependencies.bufferer_task_executor,
                                                .p7_gateway           = base_dependencies.p7_gateway,
                                                .p7_last_req_notifier = base_dependencies.p7_last_req_notifier});
  }

  auto decorators = fapi::create_decorators(decorator_cfg);

  // No decorators, so return the sector adaptor.
  if (!decorators) {
    return std::make_unique<mac_fapi_p7_sector_fastpath_adaptor_impl>(config, std::move(dependencies));
  }

  // Decorators present. Create new dependencies to point to the decorators.
  mac_fapi_p7_sector_fastpath_adaptor_impl_dependencies adaptor_dependencies = {
      .base_dependencies =
          mac_fapi_p7_sector_fastpath_adaptor_dependencies{
              .p7_gateway             = decorators->get_p7_requests_gateway(),
              .p7_last_req_notifier   = decorators->get_p7_last_request_notifier(),
              .pm_mapper              = std::move(base_dependencies.pm_mapper),
              .part2_mapper           = std::move(base_dependencies.part2_mapper),
              .bufferer_task_executor = base_dependencies.bufferer_task_executor},
      .slot_handler = dependencies.slot_handler,
  };

  auto sector_adaptor =
      std::make_unique<mac_fapi_p7_sector_fastpath_adaptor_impl>(config, std::move(adaptor_dependencies));

  // Return the wrapper that keeps the life of the decorators.
  return std::make_unique<mac_fapi_adaptor_wrapper>(std::move(sector_adaptor), std::move(decorators));
}
