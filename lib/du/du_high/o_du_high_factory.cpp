/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/du/du_high/o_du_high_factory.h"
#include "o_du_high_impl.h"
#include "ocudu/du/du_high/du_high_clock_controller.h"
#include "ocudu/du/du_high/du_high_factory.h"
#include "ocudu/du/du_high/o_du_high_config.h"
#include "ocudu/e2/e2_du_factory.h"
#include "ocudu/fapi/decorator_factory.h"
#include "ocudu/fapi/p5/p5_requests_gateway.h"
#include "ocudu/fapi_adaptor/mac/mac_fapi_fastpath_adaptor_factory.h"
#include "ocudu/fapi_adaptor/precoding_matrix_table_generator.h"
#include "ocudu/fapi_adaptor/uci_part2_correspondence_generator.h"
#include "ocudu/ran/band_helper.h"
#include "ocudu/ran/ssb/ssb_mapping.h"

using namespace ocudu;
using namespace odu;

static fapi_adaptor::mac_fapi_p5_sector_fastpath_adaptor_dependencies
generate_mac_fapi_p5_sector_adaptor_dependencies(const o_du_high_sector_dependencies& sector_dependencies)
{
  return {.logger             = ocudulog::fetch_basic_logger("FAPI"),
          .p5_gateway         = sector_dependencies.p5_gateway,
          .timers             = sector_dependencies.timer_mng,
          .fapi_ctrl_executor = sector_dependencies.fapi_ctrl_executor,
          .mac_ctrl_executor  = sector_dependencies.mac_ctrl_executor};
}

static fapi_adaptor::mac_fapi_p5_sector_fastpath_adaptor_config
generate_fapi_p5_cell_config(const du_cell_config& du_cell)
{
  fapi::cell_configuration cell_cfg;
  const subcarrier_spacing scs_common = du_cell.ran.dl_cfg_common.init_dl_bwp.generic_params.scs;

  cell_cfg.scs_common = scs_common;
  cell_cfg.cp         = du_cell.ran.dl_cfg_common.init_dl_bwp.generic_params.cp;

  cell_cfg.duplex = (du_cell.ran.tdd_ul_dl_cfg_common) ? duplex_mode::TDD : duplex_mode::FDD;
  cell_cfg.pci    = du_cell.ran.pci;

  unsigned numerology       = to_numerology_value(scs_common);
  unsigned grid_size_bw_prb = band_helper::get_n_rbs_from_bw(
      du_cell.ran.dl_carrier.carrier_bw,
      scs_common,
      band_helper::get_freq_range(band_helper::get_band_from_dl_arfcn(du_cell.ran.dl_carrier.arfcn_f_ref)));

  cell_cfg.carrier_cfg.dl_bandwidth = bs_channel_bandwidth_to_MHz(du_cell.ran.dl_carrier.carrier_bw);
  cell_cfg.carrier_cfg.ul_bandwidth = bs_channel_bandwidth_to_MHz(du_cell.ran.ul_carrier.carrier_bw);

  cell_cfg.carrier_cfg.dl_f_ref_arfcn = du_cell.ran.dl_carrier.arfcn_f_ref.value();
  cell_cfg.carrier_cfg.ul_f_ref_arfcn = du_cell.ran.ul_carrier.arfcn_f_ref.value();

  // NOTE; for now we only need to fill the nof_prb_ul_grid and nof_prb_dl_grid for the common SCS.
  cell_cfg.carrier_cfg.dl_grid_size             = {};
  cell_cfg.carrier_cfg.dl_grid_size[numerology] = grid_size_bw_prb;
  cell_cfg.carrier_cfg.ul_grid_size             = {};
  cell_cfg.carrier_cfg.ul_grid_size[numerology] = grid_size_bw_prb;

  // Number of transmit and receive antenna ports.
  cell_cfg.carrier_cfg.num_tx_ant     = du_cell.ran.dl_carrier.nof_ant;
  cell_cfg.carrier_cfg.num_rx_ant     = du_cell.ran.ul_carrier.nof_ant;
  cell_cfg.carrier_cfg.dmrs_typeA_pos = du_cell.ran.dmrs_typeA_pos;

  cell_cfg.ssb_cfg              = du_cell.ran.ssb_cfg;
  cell_cfg.tdd_ul_dl_cfg_common = du_cell.ran.tdd_ul_dl_cfg_common;
  report_error_if_not(du_cell.ran.ul_cfg_common.init_ul_bwp.rach_cfg_common, "RACH configuration not present");

  cell_cfg.prach_cfg = *du_cell.ran.ul_cfg_common.init_ul_bwp.rach_cfg_common;

  return {cell_cfg};
}

static fapi_adaptor::mac_fapi_fastpath_adaptor_config
generate_fapi_fastpath_adaptor_config(const o_du_high_config& config)
{
  fapi_adaptor::mac_fapi_fastpath_adaptor_config out_config;

  for (unsigned i = 0, e = config.du_hi.ran.cells.size(); i != e; ++i) {
    const auto&              du_cell    = config.du_hi.ran.cells[i];
    const subcarrier_spacing scs_common = du_cell.ran.dl_cfg_common.init_dl_bwp.generic_params.scs;
    unsigned                 nof_prb    = get_max_Nprb(
        du_cell.ran.dl_carrier.carrier_bw, scs_common, band_helper::get_freq_range(du_cell.ran.dl_carrier.band));
    fapi_adaptor::mac_fapi_p7_sector_fastpath_adaptor_config p7_cfg = {.sector_id     = i,
                                                                       .cell_nof_prbs = nof_prb,
                                                                       .scs           = scs_common,
                                                                       .log_level     = config.fapi.log_level,
                                                                       .l2_nof_slots_ahead =
                                                                           config.fapi.l2_nof_slots_ahead};

    out_config.sectors.push_back({.p5_config = generate_fapi_p5_cell_config(du_cell), .p7_config = p7_cfg});
  }

  return out_config;
}

static fapi_adaptor::mac_fapi_p7_sector_fastpath_adaptor_dependencies
generate_mac_fapi_p7_sector_adaptor_dependencies(const o_du_high_sector_dependencies& sector_dependencies,
                                                 unsigned                             nof_tx_antennas,
                                                 unsigned                             sector)
{
  return {.p7_gateway             = sector_dependencies.p7_gateway,
          .p7_last_req_notifier   = sector_dependencies.p7_last_req_notifier,
          .pm_mapper              = std::move(std::get<std::unique_ptr<fapi_adaptor::precoding_matrix_mapper>>(
              fapi_adaptor::generate_precoding_matrix_tables(nof_tx_antennas, sector))),
          .part2_mapper           = std::move(std::get<std::unique_ptr<fapi_adaptor::uci_part2_correspondence_mapper>>(
              fapi_adaptor::generate_uci_part2_correspondence(1))),
          .bufferer_task_executor = sector_dependencies.fapi_executor};
}

static fapi_adaptor::mac_fapi_fastpath_adaptor_dependencies
generate_fapi_fastpath_adaptor_dependencies(const o_du_high_config& config, o_du_high_dependencies& odu_dependencies)
{
  fapi_adaptor::mac_fapi_fastpath_adaptor_dependencies out_dependencies;

  for (unsigned i = 0, e = config.du_hi.ran.cells.size(); i != e; ++i) {
    const auto& sector_dependencies = odu_dependencies.sectors[i];
    out_dependencies.sectors.push_back(
        {.p5_dependencies = generate_mac_fapi_p5_sector_adaptor_dependencies(sector_dependencies),
         .p7_dependencies = generate_mac_fapi_p7_sector_adaptor_dependencies(
             sector_dependencies, config.du_hi.ran.cells[i].ran.dl_carrier.nof_ant, i)});
  }

  return out_dependencies;
}

std::unique_ptr<o_du_high> ocudu::odu::make_o_du_high(const o_du_high_config&  config,
                                                      o_du_high_dependencies&& odu_dependencies)
{
  o_du_high_impl_dependencies dependencies;
  ocudulog::basic_logger*     logger = &ocudulog::fetch_basic_logger("DU");
  dependencies.logger                = logger;
  dependencies.fapi_fastpath_adaptor = fapi_adaptor::create_mac_fapi_fastpath_adaptor_factory()->create(
      generate_fapi_fastpath_adaptor_config(config),
      generate_fapi_fastpath_adaptor_dependencies(config, odu_dependencies));
  dependencies.metrics_notifier = odu_dependencies.du_hi.du_notifier;

  logger->debug("FAPI adaptors created successfully");

  odu::du_high_configuration du_hi_cfg = config.du_hi;

  auto odu = std::make_unique<o_du_high_impl>(config.du_hi.ran.cells.size(), std::move(dependencies));

  // Resolve dependencies for DU-high.
  odu_dependencies.du_hi.phy_adapter = &odu->get_mac_result_notifier();
  odu_dependencies.du_hi.du_notifier = &odu->get_du_metrics_notifier();

  if (!odu_dependencies.e2_client) {
    odu->set_du_high(make_du_high(du_hi_cfg, odu_dependencies.du_hi));
    logger->info("DU created successfully");

    return odu;
  }

  auto du_hi = make_du_high(du_hi_cfg, odu_dependencies.du_hi);

  auto e2agent = create_e2_du_agent(config.e2ap_config,
                                    *odu_dependencies.e2_client,
                                    odu_dependencies.e2_du_metric_iface,
                                    &du_hi->get_f1ap_du(),
                                    &du_hi->get_du_configurator(),
                                    timer_factory{odu_dependencies.du_hi.timer_ctrl->get_timer_manager(),
                                                  odu_dependencies.du_hi.exec_mapper->du_e2_executor()},
                                    odu_dependencies.du_hi.exec_mapper->du_e2_executor());

  odu->set_e2_agent(std::move(e2agent));
  odu->set_du_high(std::move(du_hi));
  logger->info("DU created successfully");

  return odu;
}
