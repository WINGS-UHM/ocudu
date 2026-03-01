// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/isac/isac_sink.h"
#include "ocudu/phy/metrics/phy_metrics_notifiers.h"
#include "ocudu/phy/metrics/phy_metrics_reports.h"
#include "ocudu/phy/upper/equalization/channel_equalizer.h"
#include "ocudu/support/resource_usage/scoped_resource_usage.h"
#include <chrono>
#include <complex>
#include <memory>

namespace ocudu {

/// Channel precoder metric decorator.
class phy_metrics_channel_equalizer_decorator : public channel_equalizer
{
public:
  /// Creates a channel equalizer decorator from a base instance and a metric notifier.
  phy_metrics_channel_equalizer_decorator(std::unique_ptr<channel_equalizer> base_equalizer_,
                                          channel_equalizer_metric_notifier& notifier_) :
    base_equalizer(std::move(base_equalizer_)), notifier(notifier_)
  {
    ocudu_assert(base_equalizer, "Invalid channel equalizer.");
  }

  // See interface for documentation.
  void equalize(span<cf_t>                       eq_symbols,
                span<float>                      eq_noise_vars,
                const re_buffer_reader<cbf16_t>& ch_symbols,
                const ch_est_list&               ch_estimates,
                span<const float>                noise_var_estimates,
                float                            tx_scaling) override
  {
    channel_equalizer_metrics metrics;
    {
      // Use scoped resource usage class to measure CPU usage of this block.
      resource_usage_utils::scoped_resource_usage rusage_tracker(metrics.measurements);
      base_equalizer->equalize(eq_symbols, eq_noise_vars, ch_symbols, ch_estimates, noise_var_estimates, tx_scaling);
      // --- ISAC tap (lightweight, no heap) ---
      {
        // Timestamp (ns) for Python plotting alignment.
        const auto     now       = std::chrono::high_resolution_clock::now().time_since_epoch();
        const uint64_t tstamp_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

        // Downsample factors (tune as needed).
        constexpr unsigned CONST_DECIM = 4;
        constexpr unsigned CSI_DECIM   = 4;

        // Local fixed buffers (stack) - small + deterministic.
        float ci[ocudu::isac_snapshot::MAX_CONST];
        float cq[ocudu::isac_snapshot::MAX_CONST];
        float cmag[ocudu::isac_snapshot::MAX_CSI];
        float cph[ocudu::isac_snapshot::MAX_CSI];

        // Constellation: eq_symbols (complex float).
        unsigned       n_const      = 0;
        const unsigned max_const_in = (unsigned)eq_symbols.size();
        for (unsigned i = 0; i < max_const_in && n_const < ocudu::isac_snapshot::MAX_CONST; i += CONST_DECIM) {
          const cf_t s = eq_symbols[i];
          ci[n_const]  = (float)s.real();
          cq[n_const]  = (float)s.imag();
          ++n_const;
        }

        // CSI: use rx_port=0, layer=0 for now.
        unsigned       n_csi  = 0;
        const unsigned nof_re = ch_estimates.get_nof_re();
        if (ch_estimates.get_nof_rx_ports() > 0 && ch_estimates.get_nof_tx_layers() > 0 && nof_re > 0) {
          auto           h      = ch_estimates.get_channel(0, 0); // span<const cbf16_t>
          const unsigned max_re = (unsigned)h.size();
          for (unsigned k = 0; k < max_re && n_csi < ocudu::isac_snapshot::MAX_CSI; k += CSI_DECIM) {
            const cf_t                hf = to_cf(h[k]);
            const std::complex<float> z{(float)hf.real(), (float)hf.imag()};
            cmag[n_csi] = std::abs(z);
            cph[n_csi]  = std::arg(z);
            ++n_csi;
          }
        }

        // Publish snapshot to sink (double-buffer, no allocations).
        ocudu::get_isac_sink().write(ci, cq, n_const, cmag, cph, n_csi, tstamp_ns);
      }
      // --- end ISAC tap ---
    }
    metrics.nof_re     = ch_estimates.get_nof_re();
    metrics.nof_layers = ch_estimates.get_nof_tx_layers();
    metrics.nof_ports  = ch_estimates.get_nof_rx_ports();
    notifier.on_new_metric(metrics);
  }

  // See interface for documentation.
  bool is_supported(unsigned nof_ports, unsigned nof_layers) override
  {
    return base_equalizer->is_supported(nof_ports, nof_layers);
  }

private:
  std::unique_ptr<channel_equalizer> base_equalizer;
  channel_equalizer_metric_notifier& notifier;
};

} // namespace ocudu
