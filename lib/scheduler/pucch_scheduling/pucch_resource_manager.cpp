// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "pucch_resource_manager.h"
#include "pucch_collision_manager.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"
#include "ocudu/scheduler/resource_grid_util.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;

/////////////    RESOURCE MANAGER     /////////////

/////////////   Public methods   /////////////

pucch_resource_manager::pucch_resource_manager(const cell_configuration& cell_cfg_) :
  cell_cfg(cell_cfg_),
  collision_manager(cell_cfg_),
  slots_ctx(get_allocator_ring_size_gt_min(get_max_slot_ul_alloc_delay(cell_cfg_.ntn_cs_koffset)),
            {static_vector<rnti_t, pucch_constants::MAX_NOF_CELL_DED_RESOURCES>(
                cell_cfg_.bwp_res[to_bwp_id(0)].ul().pucch.resources.size(),
                rnti_t::INVALID_RNTI)})
{
}

void pucch_resource_manager::slot_indication(slot_point slot_tx)
{
  // If last_sl_ind is not valid (not initialized), then the check sl_tx == last_sl_ind + 1 does not matter.
  ocudu_sanity_check(not last_sl_ind.valid() or slot_tx == last_sl_ind + 1, "Detected a skipped slot");

  // Update Slot.
  last_sl_ind = slot_tx;

  // Clear previous slot context.
  auto& ctx = slots_ctx[(slot_tx - 1).count()];
  std::fill(ctx.ues_using_pucch_res.begin(), ctx.ues_using_pucch_res.end(), rnti_t::INVALID_RNTI);

  collision_manager.slot_indication(slot_tx);
}

void pucch_resource_manager::stop()
{
  for (auto& ctx : slots_ctx) {
    std::fill(ctx.ues_using_pucch_res.begin(), ctx.ues_using_pucch_res.end(), rnti_t::INVALID_RNTI);
  }
  last_sl_ind = {};
  collision_manager.stop();
}

bool pucch_resource_manager::reserve_harq_common_resource(cell_slot_resource_grid& ul_res_grid,
                                                          slot_point               sl,
                                                          size_t                   r_pucch)

{
  return collision_manager.alloc_common(ul_res_grid, sl, r_pucch).has_value();
}

void pucch_resource_manager::release_harq_common_resource(cell_slot_resource_grid& ul_res_grid,
                                                          slot_point               sl,
                                                          size_t                   r_pucch)
{
  collision_manager.free_common(ul_res_grid, sl, r_pucch);
}

/////////////   UE Reservation Guard   /////////////

pucch_resource_manager::ue_reservation_guard::ue_reservation_guard(pucch_resource_manager*       parent_,
                                                                   cell_slot_resource_allocator& slot_alloc,
                                                                   rnti_t                        rnti_,
                                                                   const ue_cell_configuration&  ue_cfg_) :
  parent(parent_),
  ul_res_grid(slot_alloc.ul_res_grid),
  res_params(parent_->cell_cfg.params.init_bwp.pucch.resources),
  cell_pucch_cfg(parent_->cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch),
  rnti(rnti_),
  sl(slot_alloc.slot),
  ue_bwp_cfg(*ue_cfg_.bwp(to_bwp_id(0)).ul.ue_cfg())
{
  ocudu_sanity_check(parent != nullptr, "PUCCH Resource Manager pointer cannot be null");
  ocudu_sanity_check(sl < parent->last_sl_ind + RES_MANAGER_RING_BUFFER_SIZE,
                     "PUCCH being allocated too far into the future");
  ocudu_sanity_check(rnti != rnti_t::INVALID_RNTI, "RNTI cannot be invalid");
}

pucch_resource_manager::ue_reservation_guard::~ue_reservation_guard()
{
  rollback();
}

pucch_harq_resource_alloc_record
pucch_resource_manager::ue_reservation_guard::reserve_harq_set_0_resource_next_available()
{
  return reserve_next_harq_res_available<0>();
}

pucch_harq_resource_alloc_record
pucch_resource_manager::ue_reservation_guard::reserve_harq_set_1_resource_next_available()
{
  return reserve_next_harq_res_available<1>();
}

const pucch_resource*
pucch_resource_manager::ue_reservation_guard::reserve_harq_set_0_resource_by_res_indicator(unsigned d_pri)

{
  return reserve_harq_resource_by_res_indicator<0>(d_pri);
}

const pucch_resource*
pucch_resource_manager::ue_reservation_guard::reserve_harq_set_1_resource_by_res_indicator(unsigned d_pri)
{
  return reserve_harq_resource_by_res_indicator<1>(d_pri);
}

const pucch_resource* pucch_resource_manager::ue_reservation_guard::reserve_sr_resource()
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");

  // Get resource list of wanted slot.
  auto& ctx = parent->slots_ctx[sl.count()];

  // Check if the wanted resource is already allocated to another UE in this slot.
  const unsigned sr_cell_res_id = res_params.get_sr_cell_res_idx(ue_bwp_cfg.pucch.sr_res_id);
  auto&          res_rnti       = ctx.ues_using_pucch_res[sr_cell_res_id];
  if (res_rnti != rnti_t::INVALID_RNTI and res_rnti != rnti) {
    return nullptr;
  }

  // If the PUCCH res with correct ID was not allocated to the UE's RNTI, allocate it to this RNTI; otherwise, it means
  // the resource had already been allocated, just return it.
  if (res_rnti != rnti) {
    // Check for collisions.
    if (not parent->collision_manager.alloc_ded(ul_res_grid, sl, sr_cell_res_id).has_value()) {
      return nullptr;
    }
    res_rnti                                                     = rnti;
    reservations[static_cast<unsigned>(resource_usage_type::sr)] = {sr_cell_res_id};
  }

  return &cell_pucch_cfg.resources[sr_cell_res_id];
}

const pucch_resource* pucch_resource_manager::ue_reservation_guard::reserve_csi_resource()
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");

  // Get resource list of wanted slot.
  auto& ctx = parent->slots_ctx[sl.count()];

  // Check if the wanted resource is already allocated to another UE in this slot.
  const unsigned csi_cell_res_id = res_params.get_csi_cell_res_idx(ue_bwp_cfg.periodic_csi_report->pucch_res_id);
  auto&          res_rnti        = ctx.ues_using_pucch_res[csi_cell_res_id];
  if (res_rnti != rnti_t::INVALID_RNTI and res_rnti != rnti) {
    return nullptr;
  }

  // If the PUCCH res with correct ID was not allocated to the UE's RNTI, allocate it to this RNTI; otherwise, it means
  // the resource had already been allocated, just return it.
  if (res_rnti != rnti) {
    // Check for collisions.
    if (not parent->collision_manager.alloc_ded(ul_res_grid, sl, csi_cell_res_id).has_value()) {
      return nullptr;
    }
    res_rnti                                                      = rnti;
    reservations[static_cast<unsigned>(resource_usage_type::csi)] = {csi_cell_res_id};
  }

  return &cell_pucch_cfg.resources[csi_cell_res_id];
}

const pucch_resource* pucch_resource_manager::ue_reservation_guard::peek_sr_resource() const
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");

  const unsigned sr_cell_res_id = res_params.get_sr_cell_res_idx(ue_bwp_cfg.pucch.sr_res_id);
  return &cell_pucch_cfg.resources[sr_cell_res_id];
}

const pucch_resource* pucch_resource_manager::ue_reservation_guard::peek_csi_resource() const
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");

  const unsigned csi_cell_res_id = res_params.get_csi_cell_res_idx(ue_bwp_cfg.periodic_csi_report->pucch_res_id);
  return &cell_pucch_cfg.resources[csi_cell_res_id];
}

bool pucch_resource_manager::ue_reservation_guard::release_harq_set_0_resource()
{
  return release_harq_resource<0>();
}

bool pucch_resource_manager::ue_reservation_guard::release_harq_set_1_resource()
{
  return release_harq_resource<1>();
}

bool pucch_resource_manager::ue_reservation_guard::release_sr_resource()
{
  ocudu_assert(parent != nullptr, "Trying to release a PUCCH resource after commit has been called");

  // Get resource list of wanted slot.
  auto& ctx = parent->slots_ctx[sl.count()];

  // Check if the resource is allocated to this RNTI.
  const unsigned sr_cell_res_id = res_params.get_sr_cell_res_idx(ue_bwp_cfg.pucch.sr_res_id);
  if (ctx.ues_using_pucch_res[sr_cell_res_id] != rnti) {
    return false;
  }

  // Release the resource.
  ctx.ues_using_pucch_res[sr_cell_res_id] = rnti_t::INVALID_RNTI;
  parent->collision_manager.free_ded(ul_res_grid, sl, sr_cell_res_id);
  reservations[static_cast<unsigned>(resource_usage_type::sr)].cell_res_id = std::nullopt;
  return true;
}

bool pucch_resource_manager::ue_reservation_guard::release_csi_resource()
{
  ocudu_assert(parent != nullptr, "Trying to release a PUCCH resource after commit has been called");

  // Get resource list of wanted slot.
  auto& ctx = parent->slots_ctx[sl.count()];

  // Check if the resource is allocated to this RNTI.
  const unsigned csi_cell_res_id = res_params.get_csi_cell_res_idx(ue_bwp_cfg.periodic_csi_report->pucch_res_id);
  if (ctx.ues_using_pucch_res[csi_cell_res_id] != rnti) {
    return false;
  }

  // Release the resource.
  ctx.ues_using_pucch_res[csi_cell_res_id] = rnti_t::INVALID_RNTI;
  parent->collision_manager.free_ded(ul_res_grid, sl, csi_cell_res_id);
  reservations[static_cast<unsigned>(resource_usage_type::csi)].cell_res_id = std::nullopt;
  return true;
}

template <unsigned ResourceSetId>
pucch_harq_resource_alloc_record pucch_resource_manager::ue_reservation_guard::reserve_next_harq_res_available()
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");

  // Get context of wanted slot.
  auto& ctx = parent->slots_ctx[sl.count()];

  // If a resource is already allocated to this RNTI, use it.
  std::optional<unsigned> available_res;
  const auto              res_set_cfg_id = ue_bwp_cfg.pucch.res_set_cfg_id;
  const unsigned          res_set_size =
      ResourceSetId == 0 ? res_params.res_set_0_size.value() : res_params.res_set_1_size.value();
  for (uint8_t r_pucch = 0; r_pucch != res_set_size; ++r_pucch) {
    unsigned cell_res_id = res_params.get_res_set_cell_res_idx<ResourceSetId>(res_set_cfg_id, r_pucch);

    auto& res_rnti = ctx.ues_using_pucch_res[cell_res_id];
    if (res_rnti == rnti) {
      available_res = r_pucch;
      break;
    }
  }

  if (not available_res.has_value()) {
    // Else, try to allocate the first available resource.
    for (unsigned r_pucch = 0; r_pucch != res_set_size; ++r_pucch) {
      unsigned cell_res_id = res_params.get_res_set_cell_res_idx<ResourceSetId>(res_set_cfg_id, r_pucch);
      auto&    res_rnti    = ctx.ues_using_pucch_res[cell_res_id];
      if (res_rnti == rnti_t::INVALID_RNTI and
          parent->collision_manager.alloc_ded(ul_res_grid, sl, cell_res_id).has_value()) {
        ctx.ues_using_pucch_res[cell_res_id] = rnti;
        const unsigned usage_type_idx = ResourceSetId == 0 ? static_cast<unsigned>(resource_usage_type::harq_set_0)
                                                           : static_cast<unsigned>(resource_usage_type::harq_set_1);
        reservations[usage_type_idx]  = {cell_res_id};
        available_res                 = r_pucch;
        break;
      }
    }
  }

  // If an available resource was found, return it.
  if (available_res.has_value()) {
    const unsigned cell_res_id = res_params.get_res_set_cell_res_idx<ResourceSetId>(res_set_cfg_id, *available_res);
    return pucch_harq_resource_alloc_record{.resource            = &cell_pucch_cfg.resources[cell_res_id],
                                            .pucch_res_indicator = static_cast<uint8_t>(available_res.value())};
  }
  return pucch_harq_resource_alloc_record{.resource = nullptr};
}

template <unsigned ResourceSetId>
const pucch_resource*
pucch_resource_manager::ue_reservation_guard::reserve_harq_resource_by_res_indicator(unsigned d_pri)
{
  ocudu_assert(parent != nullptr, "Trying to make a new PUCCH resource reservation after commit has been called");
  const unsigned res_set_size =
      ResourceSetId == 0 ? res_params.res_set_0_size.value() : res_params.res_set_1_size.value();
  const unsigned max_pri = parent->cell_cfg.is_pucch_f0_and_f2()
                               ? res_set_size + (ue_bwp_cfg.periodic_csi_report.has_value() ? 2U : 1U)
                               : res_set_size;
  // Make sure the resource indicator points to a valid resource.
  if (d_pri >= max_pri) {
    return nullptr;
  }

  // Get resource list of wanted slot.
  slot_context& ctx = parent->slots_ctx[sl.count()];

  // Get PUCCH resource ID from the PUCCH resource set.
  // [Implementation-defined] We assume at most 8 resources per resource set. If this is the case, r_pucch = d_pri.

  // For Format 0 and Format 2, the resources indexed by PUCCH res. indicators >= res_set_size are reserved for CSI and
  // SR slots. In the case, we don't need to reserve these in the PUCCH resource manager, we only need to return the
  // resources.
  if (parent->cell_cfg.is_pucch_f0_and_f2() and d_pri >= res_set_size) {
    if (ResourceSetId == 0) {
      if (d_pri == res_set_size) {
        return &cell_pucch_cfg.resources[res_params.get_sr_cell_res_idx(ue_bwp_cfg.pucch.sr_res_id)];
      }
      return &cell_pucch_cfg
                  .resources[res_params.get_csi_f0_cell_res_idx(ue_bwp_cfg.periodic_csi_report->pucch_res_id)];
    }
    // Resource Set ID 1.
    if (d_pri == res_set_size) {
      return &cell_pucch_cfg.resources[res_params.get_sr_f2_cell_res_idx(ue_bwp_cfg.pucch.sr_res_id)];
    }
    return &cell_pucch_cfg.resources[res_params.get_csi_cell_res_idx(ue_bwp_cfg.periodic_csi_report->pucch_res_id)];
  }

  // Check first if the wanted PUCCH resource is available.
  const unsigned cell_res_id =
      res_params.get_res_set_cell_res_idx<ResourceSetId>(ue_bwp_cfg.pucch.res_set_cfg_id, d_pri);
  auto& res_rnti = ctx.ues_using_pucch_res[cell_res_id];
  if (res_rnti != rnti_t::INVALID_RNTI and res_rnti != rnti) {
    return nullptr;
  }

  if (res_rnti == rnti) {
    // If the resource is already allocated to this RNTI, just return it.
    return &cell_pucch_cfg.resources[cell_res_id];
  }

  // Allocate the resource to this RNTI.
  if (not parent->collision_manager.alloc_ded(ul_res_grid, sl, cell_res_id).has_value()) {
    return nullptr;
  }
  res_rnti                      = rnti;
  const unsigned usage_type_idx = ResourceSetId == 0 ? static_cast<unsigned>(resource_usage_type::harq_set_0)
                                                     : static_cast<unsigned>(resource_usage_type::harq_set_1);
  reservations[usage_type_idx]  = {cell_res_id};
  return &cell_pucch_cfg.resources[cell_res_id];
}

template <unsigned ResourceSetId>
bool pucch_resource_manager::ue_reservation_guard::release_harq_resource()
{
  ocudu_assert(parent != nullptr, "Trying to release a PUCCH resource after commit has been called");

  const unsigned res_set_size =
      ResourceSetId == 0 ? res_params.res_set_0_size.value() : res_params.res_set_1_size.value();

  // Get resource list of wanted slot.
  slot_context& ctx = parent->slots_ctx[sl.count()];

  for (unsigned r_pucch = 0; r_pucch != res_set_size; ++r_pucch) {
    const unsigned cell_res_id =
        res_params.get_res_set_cell_res_idx<ResourceSetId>(ue_bwp_cfg.pucch.res_set_cfg_id, r_pucch);
    auto& res_rnti = ctx.ues_using_pucch_res[cell_res_id];

    if (res_rnti == rnti) {
      // Release the resource.
      res_rnti = rnti_t::INVALID_RNTI;
      parent->collision_manager.free_ded(ul_res_grid, sl, cell_res_id);
      const unsigned usage_type_idx = ResourceSetId == 0 ? static_cast<unsigned>(resource_usage_type::harq_set_0)
                                                         : static_cast<unsigned>(resource_usage_type::harq_set_1);
      reservations[usage_type_idx].cell_res_id = std::nullopt;
      return true;
    }
  }

  return false;
}

void pucch_resource_manager::ue_reservation_guard::commit()
{
  ocudu_assert(parent != nullptr, "Trying to commit PUCCH resource reservations after commit has been called");

  // If both HARQ resource sets have been reserved, only keep the resource in PUCCH Resource Set ID 1.
  if (reservations[static_cast<unsigned>(resource_usage_type::harq_set_0)].cell_res_id.has_value() and
      reservations[static_cast<unsigned>(resource_usage_type::harq_set_1)].cell_res_id.has_value()) {
    // Release HARQ Resource Set ID 0 reservation.
    const unsigned cell_res_id =
        reservations[static_cast<unsigned>(resource_usage_type::harq_set_0)].cell_res_id.value();
    auto& ctx                            = parent->slots_ctx[sl.count()];
    ctx.ues_using_pucch_res[cell_res_id] = rnti_t::INVALID_RNTI;
    parent->collision_manager.free_ded(ul_res_grid, sl, cell_res_id);
    reservations[static_cast<unsigned>(resource_usage_type::harq_set_0)].cell_res_id = std::nullopt;
  }

  // Clear parent pointer to avoid further reservations after commit.
  parent = nullptr;
}

void pucch_resource_manager::ue_reservation_guard::rollback()
{
  // If parent is nullptr, it means commit() was already called.
  if (parent != nullptr) {
    // Release all reservations made so far.
    for (auto& res : reservations) {
      if (res.cell_res_id.has_value()) {
        // Get resource list of wanted slot.
        auto& ctx = parent->slots_ctx[sl.count()];
        ocudu_assert(res.cell_res_id.value() < ctx.ues_using_pucch_res.size(),
                     "rnti={}: PUCCH resource index exceeds the size of the cell resource array",
                     rnti);

        // Release the resource.
        ctx.ues_using_pucch_res[*res.cell_res_id] = rnti_t::INVALID_RNTI;
        parent->collision_manager.free_ded(ul_res_grid, sl, *res.cell_res_id);
      }
    }
  }
}
