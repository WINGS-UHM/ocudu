// UHM WINGS Fake Base Station Research

#pragma once

#include "config.h"
#include "types.h"

namespace ocudu::fbs {

void run_negative_ue_context_release(const injector_config& cfg, const runtime_options& opts);
void run_negative_gnb_control(const injector_config& cfg, const runtime_options& opts);
void run_negative_malformed_stub(const injector_config& cfg, const runtime_options& opts);

} // namespace ocudu::fbs
