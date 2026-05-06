// UHM WINGS Fake Base Station Research

#pragma once

#include "config.h"
#include "ngap_decoder.h"
#include "types.h"
#include <string>
#include <vector>

namespace ocudu::fbs {

class observed_context_table
{
public:
  void observe(const ngap_message_summary& summary, const std::string& source_ip, const std::string& destination_ip);
  const std::vector<observed_ue_context>& contexts() const { return entries; }
  void export_json(const std::string& path) const;

private:
  std::vector<observed_ue_context> entries;
};

std::vector<observed_ue_context> run_passive_sniffer(const injector_config& cfg,
                                                     unsigned               max_packets,
                                                     unsigned               duration_seconds,
                                                     const std::string&     export_path);

std::vector<observed_ue_context> load_contexts_from_json(const std::string& path);

} // namespace ocudu::fbs
