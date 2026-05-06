// UHM WINGS Fake Base Station Research

#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace ocudu::fbs {

inline constexpr const char* lab_override_confirmation = "I_CONFIRM_CLOSED_LAB_USE_ONLY";

injector_config load_injector_config(const std::string& path);

bool is_valid_ip_literal(const std::string& value);
bool is_private_or_lab_scoped_ip(const std::string& value);
bool is_amf_allowlisted(const injector_config& cfg);
bool is_interface_allowlisted(const injector_config& cfg);

void validate_config_for_mode(const injector_config& cfg, const runtime_options& opts, harness_mode mode);
void print_run_banner(const injector_config& cfg, const runtime_options& opts, harness_mode mode, bool sending_enabled);

harness_mode harness_mode_from_string(const std::string& value);

} // namespace ocudu::fbs
