// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "apps/services/cmdline/cmdline_command.h"
#include "apps/services/cmdline/cmdline_command_dispatcher_utils.h"
#include "ocudu/adt/expected.h"
#include "ocudu/cu_cp/cu_cp_command_handler.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/rnti.h"
#include <chrono>
#include <vector>

namespace ocudu {

/// Application command to trigger a handover.
class handover_app_command : public app_services::cmdline_command
{
  ocucp::cu_cp_command_handler& cu_cp;

public:
  explicit handover_app_command(ocucp::cu_cp_command_handler& cu_cp_) : cu_cp(cu_cp_) {}

  // See interface for documentation.
  std::string_view get_name() const override { return "ho"; }

  // See interface for documentation.
  std::string_view get_description() const override { return " <serving pci> <rnti> <target pci>: force UE handover"; }

  // See interface for documentation.
  void execute(span<const std::string> args) override
  {
    if (args.size() != 3) {
      fmt::print("Invalid handover command structure. Usage: ho <serving pci> <rnti> <target pci>\n");
      return;
    }

    const auto*                     arg         = args.begin();
    expected<unsigned, std::string> serving_pci = app_services::parse_int<unsigned>(*arg);
    if (not serving_pci.has_value()) {
      fmt::print("Invalid serving PCI.\n");
      return;
    }
    ++arg;
    expected<unsigned, std::string> rnti = app_services::parse_unsigned_hex<unsigned>(*arg);
    if (not rnti.has_value()) {
      fmt::print("Invalid UE RNTI.\n");
      return;
    }
    ++arg;
    expected<unsigned, std::string> target_pci = app_services::parse_int<unsigned>(*arg);
    if (not target_pci.has_value()) {
      fmt::print("Invalid target PCI.\n");
      return;
    }

    cu_cp.get_mobility_command_handler().trigger_handover(static_cast<pci_t>(serving_pci.value()),
                                                          static_cast<rnti_t>(rnti.value()),
                                                          static_cast<pci_t>(target_pci.value()));
    fmt::print("Handover triggered for UE with pci={} rnti={} to pci={}.\n",
               serving_pci.value(),
               static_cast<rnti_t>(rnti.value()),
               target_pci.value());
  }
};

/// Application command to trigger a Conditional Handover (CHO).
class cho_app_command : public app_services::cmdline_command
{
  ocucp::cu_cp_command_handler& cu_cp;

public:
  explicit cho_app_command(ocucp::cu_cp_command_handler& cu_cp_) : cu_cp(cu_cp_) {}

  // See interface for documentation.
  std::string_view get_name() const override { return "cho"; }

  // See interface for documentation.
  std::string_view get_description() const override
  {
    return " <serving pci> <rnti> <target pci> [<target pci2> ...] [timeout <s>]: trigger conditional handover";
  }

  // See interface for documentation.
  void execute(span<const std::string> args) override
  {
    if (args.size() < 3) {
      fmt::print("Invalid cho command structure.\n");
      fmt::print("Usage: cho <serving pci> <rnti> <target pci1> [<target pci2> ... <target pci8>] [timeout <s>]\n");
      return;
    }

    const auto* arg = args.begin();

    // Parse serving PCI
    expected<unsigned, std::string> serving_pci = app_services::parse_int<unsigned>(*arg);
    if (not serving_pci.has_value()) {
      fmt::print("Invalid serving PCI.\n");
      return;
    }
    ++arg;

    // Parse RNTI
    expected<unsigned, std::string> rnti = app_services::parse_unsigned_hex<unsigned>(*arg);
    if (not rnti.has_value()) {
      fmt::print("Invalid UE RNTI.\n");
      return;
    }
    ++arg;

    // Parse target PCIs and optional timeout.
    // Timeout is specified with keyword "timeout" followed by value in seconds.
    // Examples: "cho 1 0x1234 100 200" -> 2 PCIs, default timeout
    //           "cho 1 0x1234 100 200 timeout 30" -> 2 PCIs, 30s timeout
    std::vector<pci_t>        target_pcis;
    std::chrono::milliseconds timeout{10000}; // default 10 seconds

    while (arg != args.end()) {
      if (*arg == "timeout") {
        ++arg;
        if (arg == args.end()) {
          fmt::print("Missing timeout value after 'timeout' keyword.\n");
          return;
        }
        expected<unsigned, std::string> timeout_s = app_services::parse_int<unsigned>(*arg);
        if (not timeout_s.has_value()) {
          fmt::print("Invalid timeout value: {}.\n", *arg);
          return;
        }
        timeout = std::chrono::seconds{timeout_s.value()};
        ++arg;
        break; // timeout must be last
      }

      expected<unsigned, std::string> target_pci = app_services::parse_int<unsigned>(*arg);
      if (not target_pci.has_value()) {
        fmt::print("Invalid target PCI: {}.\n", *arg);
        return;
      }
      target_pcis.push_back(static_cast<pci_t>(target_pci.value()));
      ++arg;
    }

    if (target_pcis.empty()) {
      fmt::print("No target PCIs provided.\n");
      return;
    }
    if (target_pcis.size() > 8) {
      fmt::print("Too many target PCIs: {}. Maximum 8 candidates supported per 3GPP.\n", target_pcis.size());
      return;
    }

    cu_cp.get_mobility_command_handler().trigger_conditional_handover(
        static_cast<pci_t>(serving_pci.value()), static_cast<rnti_t>(rnti.value()), target_pcis, timeout);
    fmt::print("CHO triggered for UE with pci={} rnti={} to {} target(s) with timeout={}s.\n",
               serving_pci.value(),
               static_cast<rnti_t>(rnti.value()),
               target_pcis.size(),
               std::chrono::duration_cast<std::chrono::seconds>(timeout).count());
  }
};

} // namespace ocudu
