// UHM WINGS Fake Base Station Research

#include "sniffer.h"
#include "pcap_parser.h"
#include "external/nlohmann/json.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#if defined(__linux__)
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace ocudu::fbs;
using json = nlohmann::json;

namespace {

std::string now_timestamp()
{
  const auto now      = std::chrono::system_clock::now();
  const auto now_time = std::chrono::system_clock::to_time_t(now);
  std::tm    tm       = {};
#if defined(_WIN32)
  gmtime_s(&tm, &now_time);
#else
  gmtime_r(&now_time, &tm);
#endif
  std::ostringstream os;
  os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return os.str();
}

bool same_context(const observed_ue_context& entry, const ngap_message_summary& summary)
{
  if (summary.ran_ue_ngap_id && entry.ran_ue_ngap_id && *summary.ran_ue_ngap_id == *entry.ran_ue_ngap_id) {
    return true;
  }
  if (summary.amf_ue_ngap_id && entry.amf_ue_ngap_id && *summary.amf_ue_ngap_id == *entry.amf_ue_ngap_id) {
    return true;
  }
  return false;
}

json context_to_json(const observed_ue_context& entry)
{
  json out;
  if (entry.ran_ue_ngap_id) {
    out["ran_ue_ngap_id"] = *entry.ran_ue_ngap_id;
  }
  if (entry.amf_ue_ngap_id) {
    out["amf_ue_ngap_id"] = *entry.amf_ue_ngap_id;
  }
  out["last_message_type"] = entry.last_message_type;
  out["source_ip"]         = entry.source_ip;
  out["destination_ip"]    = entry.destination_ip;
  out["timestamp"]         = entry.timestamp;
  return out;
}

observed_ue_context json_to_context(const json& in)
{
  observed_ue_context entry;
  if (in.contains("ran_ue_ngap_id")) {
    entry.ran_ue_ngap_id = in.at("ran_ue_ngap_id").get<uint64_t>();
  }
  if (in.contains("amf_ue_ngap_id")) {
    entry.amf_ue_ngap_id = in.at("amf_ue_ngap_id").get<uint64_t>();
  }
  entry.last_message_type = in.value("last_message_type", "");
  entry.source_ip         = in.value("source_ip", "");
  entry.destination_ip    = in.value("destination_ip", "");
  entry.timestamp         = in.value("timestamp", "");
  return entry;
}

} // namespace

void observed_context_table::observe(const ngap_message_summary& summary,
                                     const std::string&          source_ip,
                                     const std::string&          destination_ip)
{
  if (!summary.decode_ok || (!summary.ran_ue_ngap_id && !summary.amf_ue_ngap_id)) {
    return;
  }

  auto it = std::find_if(entries.begin(), entries.end(), [&summary](const observed_ue_context& entry) {
    return same_context(entry, summary);
  });

  observed_ue_context* entry = nullptr;
  if (it == entries.end()) {
    entries.emplace_back();
    entry = &entries.back();
  } else {
    entry = &*it;
  }

  if (summary.ran_ue_ngap_id) {
    entry->ran_ue_ngap_id = summary.ran_ue_ngap_id;
  }
  if (summary.amf_ue_ngap_id) {
    entry->amf_ue_ngap_id = summary.amf_ue_ngap_id;
  }
  entry->last_message_type = summary.message_type;
  entry->source_ip         = source_ip;
  entry->destination_ip    = destination_ip;
  entry->timestamp         = now_timestamp();
}

void observed_context_table::export_json(const std::string& path) const
{
  json out;
  out["contexts"] = json::array();
  for (const auto& entry : entries) {
    out["contexts"].push_back(context_to_json(entry));
  }

  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open context export file: " + path);
  }
  file << out.dump(2) << "\n";
}

std::vector<observed_ue_context> ocudu::fbs::load_contexts_from_json(const std::string& path)
{
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open observed context JSON: " + path);
  }
  json in = json::parse(file);

  std::vector<observed_ue_context> contexts;
  if (in.contains("contexts")) {
    for (const auto& item : in.at("contexts")) {
      contexts.push_back(json_to_context(item));
    }
  } else {
    contexts.push_back(json_to_context(in));
  }
  return contexts;
}

std::vector<observed_ue_context> ocudu::fbs::run_passive_sniffer(const injector_config& cfg,
                                                                 unsigned               max_packets,
                                                                 unsigned               duration_seconds,
                                                                 const std::string&     export_path)
{
#if !defined(__linux__)
  (void)cfg;
  (void)max_packets;
  (void)duration_seconds;
  (void)export_path;
  throw std::runtime_error("Passive sniffing requires Linux raw packet sockets");
#else
  const unsigned if_index = if_nametoindex(cfg.interface_name.c_str());
  if (if_index == 0) {
    throw std::runtime_error("Configured sniff interface was not found: " + cfg.interface_name);
  }

  const int fd = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (fd < 0) {
    throw std::runtime_error("Failed to open raw packet socket. Passive sniffing usually requires CAP_NET_RAW/root.");
  }

  sockaddr_ll bind_addr = {};
  bind_addr.sll_family   = AF_PACKET;
  bind_addr.sll_protocol = htons(ETH_P_ALL);
  bind_addr.sll_ifindex  = static_cast<int>(if_index);
  if (::bind(fd, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) != 0) {
    ::close(fd);
    throw std::runtime_error("Failed to bind raw packet socket to configured interface");
  }

  observed_context_table table;
  unsigned captured_ngap = 0;
  const auto started = std::chrono::steady_clock::now();

  std::printf("passive_sniff status=started interface=%s max_packets=%u duration_seconds=%u\n",
              cfg.interface_name.c_str(),
              max_packets,
              duration_seconds);

  while (true) {
    if (max_packets != 0 && captured_ngap >= max_packets) {
      break;
    }
    if (duration_seconds != 0 &&
        std::chrono::steady_clock::now() - started >= std::chrono::seconds(duration_seconds)) {
      break;
    }

    std::array<uint8_t, 65536> buffer = {};
    const ssize_t bytes = ::recv(fd, buffer.data(), buffer.size(), 0);
    if (bytes <= 0) {
      continue;
    }

    auto packets =
        extract_ngap_payloads_from_packet(buffer.data(), static_cast<size_t>(bytes), 1, captured_ngap);
    for (const auto& packet : packets) {
      auto summary = decode_ngap_payload(packet.payload);
      std::printf("sniffed_ngap index=%u src=%s dst=%s %s\n",
                  captured_ngap,
                  packet.source_ip.c_str(),
                  packet.destination_ip.c_str(),
                  format_summary(summary).c_str());
      table.observe(summary, packet.source_ip, packet.destination_ip);
      ++captured_ngap;
    }
  }

  ::close(fd);

  if (!export_path.empty()) {
    table.export_json(export_path);
    std::printf("context_export path=%s contexts=%zu\n", export_path.c_str(), table.contexts().size());
  }
  return table.contexts();
#endif
}
