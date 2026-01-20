/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/adt/span.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/pusch/pusch_constants.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ran/tdd/tdd_ul_dl_config.h"
#include <vector>

namespace ocudu {

class cell_configuration;
struct search_space_info;
struct pusch_config_common;

/// \brief Returns the list of all applicable PUSCH Time Domain resource indexes based on cell, UE configuration and
/// nof. symbols in PUSCH slot for a PDCCH slot.
/// \param[in] pdcch_slot Slot at which the PDCCH is supposed to be scheduled.
/// \param[in] tdd_cfg_common TDD configuration, if present.
/// \param[in] pusch_cfg_common PUSCH common configuration.
/// \param[in] dl_data_to_ul_ack List of viable k1 values.
/// \param[in] ss_info SearchSpace information.
/// \return List of PUSCH Time Domain resource indexes.
/// \remark If \c ss_info is nullptr, then minimum k1 is taken from \c cell_cfg.
static_vector<unsigned, pusch_constants::MAX_NOF_PUSCH_TD_RES_ALLOCS>
get_pusch_td_resource_indices(slot_point                                    pdcch_slot,
                              const std::optional<tdd_ul_dl_config_common>& tdd_cfg_common,
                              const pusch_config_common&                    pusch_cfg_common,
                              span<const uint8_t>                           dl_data_to_ul_ack,
                              const search_space_info*                      ss_info = nullptr);

/// \brief Returns the list circularly indexed by slot containing the list of all applicable PUSCH Time Domain resource
/// indexes per slot.
/// \param[in] scs SCS common value.
/// \param[in] tdd_cfg_common TDD configuration, if present.
/// \param[in] pusch_cfg_common PUSCH common configuration.
/// \param[in] dl_data_to_ul_ack List of viable k1 values.
/// \param[in] ss_info SearchSpace information.
/// \return A list circularly indexed by slot containing the list of all applicable PUSCH Time Domain resource indexes
/// per slot.
/// \remark The periodicity of the circularly indexed list is equal to nof. slots in a TDD period in case of TDD and 1
/// slot in case of FDD.
/// \remark If \c ss_info is nullptr, then minimum k1 is taken from \c cell_cfg.
/// \remark The list of applicable PUSCH Time Domain resources would be empty for UL slots as UL PDCCHs are scheduled
/// only over DL slots.
/// \remark In case of FDD, the list returned would contain applicable PUSCH Time Domain resource indexes for only one
/// slot.
std::vector<static_vector<unsigned, pusch_constants::MAX_NOF_PUSCH_TD_RES_ALLOCS>>
get_pusch_td_resource_indices_per_slot(subcarrier_spacing                            scs,
                                       const std::optional<tdd_ul_dl_config_common>& tdd_cfg_common,
                                       const pusch_config_common&                    pusch_cfg_common,
                                       span<const uint8_t>                           dl_data_to_ul_ack,
                                       const search_space_info*                      ss_info = nullptr);

/// Returns the list circularly indexed by slot containing the list of applicable PUSCH Time Domain resource indexes per
/// slot fairly distributed among all the PDCCH slots.
///
/// In case of UL heavy TDD pattern, this function ensures that UL PDCCH scheduling the PUSCH are fairly
/// distributed across DL slots in a TDD configuration. In all other scenarios (FDD or DL heavy TDD pattern), the
/// function behaves similar to \c get_pusch_td_resource_indices_per_slot().
///
/// \param[in] scs SCS common value.
/// \param[in] tdd_cfg_common TDD configuration, if present.
/// \param[in] pusch_cfg_common PUSCH common configuration.
/// \param[in] dl_data_to_ul_ack List of viable k1 values.
/// \param[in] ss_info SearchSpace information.
/// \return A list circularly indexed by slot containing the list of applicable PUSCH Time Domain resource indexes per
/// slot.
/// \remark The periodicity of the circularly indexed list is equal to nof. slots in a TDD period in case of TDD and 1
/// slot in case of FDD.
/// \remark If \c ss_info is nullptr, then minimum k1 is taken from \c cell_cfg.
/// \remark The list of applicable PUSCH Time Domain resources would be empty for UL slots as UL PDCCHs are scheduled
/// only over DL slots.
/// \remark In case of FDD, the list returned would contain applicable PUSCH Time Domain resource indexes for only one
/// slot.
std::vector<static_vector<unsigned, pusch_constants::MAX_NOF_PUSCH_TD_RES_ALLOCS>>
get_fairly_distributed_pusch_td_resource_indices(subcarrier_spacing                            scs,
                                                 const std::optional<tdd_ul_dl_config_common>& tdd_cfg_common,
                                                 const pusch_config_common&                    pusch_cfg_common,
                                                 span<const uint8_t>                           dl_data_to_ul_ack,
                                                 const search_space_info*                      ss_info = nullptr);

} // namespace ocudu
