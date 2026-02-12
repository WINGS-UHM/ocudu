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

#include "ocudu/gtpu/gtpu_teid_pool.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ocudulog/ocudulog.h"
#include <mutex>

namespace ocudu {

class gtpu_teid_pool_impl final : public gtpu_teid_pool
{
public:
  explicit gtpu_teid_pool_impl(uint32_t max_nof_teids_) :
    max_nof_teids(max_nof_teids_), teid_pool(max_nof_teids_), logger(ocudulog::fetch_basic_logger("GTPU"))
  {
  }

  [[nodiscard]] expected<gtpu_teid_t> request_teid() override
  {
    expected<gtpu_teid_t> teid = make_unexpected(default_error_t{});

    std::lock_guard<std::mutex> guard(pool_mutex);
    if (full_no_lock()) {
      return teid;
    }

    // Find a free teid
    bool     found   = false;
    uint16_t tmp_idx = next_teid_idx;
    for (uint16_t n = 0; n < max_nof_teids; n++) {
      tmp_idx = (next_teid_idx + n) % max_nof_teids;
      if (not teid_pool[tmp_idx]) {
        teid_pool[tmp_idx] = true;
        found              = true;
        break;
      }
    }

    if (not found) {
      return teid;
    }
    next_teid_idx = (tmp_idx + 1) % max_nof_teids;
    teid          = gtpu_teid_t{tmp_idx + GTPU_TEID_MIN.value()};
    nof_teids++;
    return teid;
  }

  [[nodiscard]] bool release_teid(gtpu_teid_t teid) override
  {
    std::lock_guard<std::mutex> guard(pool_mutex);
    uint32_t                    teid_idx = teid.value() - GTPU_TEID_MIN.value();
    if (not teid_pool[teid_idx]) {
      logger.error("Trying to free non-allocated TEID. teid={}", teid);
      return false;
    }
    teid_pool[teid_idx] = false;
    nof_teids--;
    return true;
  }

  [[nodiscard]] bool full() override
  {
    std::lock_guard<std::mutex> guard(pool_mutex);
    return full_no_lock();
  }

  uint32_t get_max_nof_teids() override { return max_nof_teids; }

private:
  [[nodiscard]] bool full_no_lock() const { return nof_teids >= max_nof_teids; }

  uint32_t       next_teid_idx = 0;
  uint32_t       nof_teids     = 0;
  const uint32_t max_nof_teids;

  std::mutex        pool_mutex;
  std::vector<bool> teid_pool;

  ocudulog::basic_logger& logger;
};
} // namespace ocudu
