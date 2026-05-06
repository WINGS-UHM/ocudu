// UHM WINGS Fake Base Station Research

#include "negative_tests.h"
#include "ngap_decoder.h"
#include "ocudu/adt/span.h"
#include "pcap_parser.h"
#include "sctp_transport.h"
#include "sniffer.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>

using namespace ocudu::fbs;

namespace {

bool actual_send_requested(const runtime_options& opts)
{
  return !opts.dry_run;
}

void validate_negative_send_gates(const injector_config& cfg, const runtime_options& opts)
{
  if (!actual_send_requested(opts)) {
    return;
  }
  if (opts.dry_run) {
    throw std::runtime_error("Refusing negative-test send: dry-run is active");
  }
  if (!opts.enable_negative_tests) {
    throw std::runtime_error("Refusing negative-test send: --enable-negative-tests is required");
  }
  if (!opts.confirm_send) {
    throw std::runtime_error("Refusing negative-test send: --confirm-send is required");
  }
  if (cfg.negative_tests.max_packet_count == 0 || cfg.negative_tests.max_packet_count > 3) {
    throw std::runtime_error("Refusing negative-test send: max_packet_count must be in the bounded range [1, 3]");
  }
}

ue_identifiers select_ue_ids(const injector_config& cfg, const runtime_options& opts)
{
  ue_identifiers ids = cfg.manual_ue_ids;
  if (!opts.context_path.empty()) {
    auto contexts = load_contexts_from_json(opts.context_path);
    for (const auto& context : contexts) {
      if (context.ran_ue_ngap_id && context.amf_ue_ngap_id) {
        ids.ran_ue_ngap_id = context.ran_ue_ngap_id;
        ids.amf_ue_ngap_id = context.amf_ue_ngap_id;
        return ids;
      }
    }
  }
  return ids;
}

byte_buffer select_or_build_ue_release_payload(const injector_config& cfg,
                                               const runtime_options& opts,
                                               const ue_identifiers&  ids)
{
  if (!opts.pcap_path.empty()) {
    auto packets = read_ngap_packets_from_pcap(opts.pcap_path);
    for (const auto& packet : packets) {
      auto summary = decode_ngap_payload(packet.payload);
      if (summary.decode_ok &&
          (summary.message_type.find("UEContextRelease") != std::string::npos ||
           summary.message_type.find("ue_context_release") != std::string::npos)) {
        if (!summary.ran_ue_ngap_id || !summary.amf_ue_ngap_id) {
          throw std::runtime_error("Selected UE-context-release PCAP message does not contain both UE identifiers");
        }
        if (!packet_endpoints_are_configured_lab_ips(packet, cfg.local_gnb_ip, cfg.amf_ip, cfg.allowlisted_amf_ips)) {
          throw std::runtime_error("Refusing replay: selected PCAP packet endpoints are not configured lab IPs");
        }
        std::printf("negative_test selected_replay_packet=%u %s\n",
                    packet.ngap_index,
                    format_summary(summary).c_str());
        byte_buffer payload{byte_buffer::fallback_allocation_tag{}};
        if (!payload.append(ocudu::span<const uint8_t>(packet.payload.data(), packet.payload.size()))) {
          throw std::runtime_error("Failed to allocate replay payload");
        }
        return payload;
      }
    }
    throw std::runtime_error("No UE-context-release-related NGAP payload found in provided PCAP");
  }

  if (!ids.amf_ue_ngap_id || !ids.ran_ue_ngap_id) {
    throw std::runtime_error("UE-context-release test requires AMF UE NGAP ID and RAN UE NGAP ID");
  }
  return build_ue_context_release_request(*ids.amf_ue_ngap_id, *ids.ran_ue_ngap_id);
}

void establish_setup_then_send_one(const injector_config& cfg, const byte_buffer& payload)
{
  const byte_buffer setup = build_ng_setup_request(cfg);
  n2_sctp_client client(cfg);
  client.connect();
  client.send_payload(setup);
  auto response = client.receive_payload(3000);
  if (!response) {
    client.mark_ng_setup_failed();
    throw std::runtime_error("Refusing negative test message: NG setup timed out");
  }
  auto setup_summary = decode_ngap_payload(*response);
  std::printf("negative_test setup_result response=\"%s\"\n", format_summary(setup_summary).c_str());
  if (!is_ng_setup_response(setup_summary)) {
    client.mark_ng_setup_failed();
    throw std::runtime_error("Refusing negative test message: NG setup did not complete successfully");
  }
  client.mark_ng_setup_accepted();

  client.send_payload(payload);
  auto test_response = client.receive_payload(3000);
  if (test_response) {
    auto summary = decode_ngap_payload(*test_response);
    std::printf("negative_test observed_amf_response %s\n", format_summary(summary).c_str());
  } else {
    std::printf("negative_test observed_amf_response none_within_timeout_ms=3000\n");
  }
}

void dry_run_payload_summary(const char* test_name, const byte_buffer& payload)
{
  auto summary = decode_ngap_payload(payload);
  std::printf("negative_test dry_run=true test=%s payload_summary=\"%s\"\n", test_name, format_summary(summary).c_str());
}

} // namespace

void ocudu::fbs::run_negative_ue_context_release(const injector_config& cfg, const runtime_options& opts)
{
  validate_config_for_mode(cfg, opts, harness_mode::negative_test);
  validate_negative_send_gates(cfg, opts);

  const ue_identifiers ids = select_ue_ids(cfg, opts);
  if (ids.ran_ue_ngap_id) {
    std::printf("negative_test ue_context ran_ue_ngap_id=%llu\n",
                static_cast<unsigned long long>(*ids.ran_ue_ngap_id));
  }
  if (ids.amf_ue_ngap_id) {
    std::printf("negative_test ue_context amf_ue_ngap_id=%llu\n",
                static_cast<unsigned long long>(*ids.amf_ue_ngap_id));
  }

  byte_buffer payload = select_or_build_ue_release_payload(cfg, opts, ids);
  dry_run_payload_summary("ue-context-release", payload);
  if (opts.dry_run) {
    std::printf("negative_test send_status=not_sent reason=dry_run\n");
    return;
  }

  establish_setup_then_send_one(cfg, payload);
}

void ocudu::fbs::run_negative_gnb_control(const injector_config& cfg, const runtime_options& opts)
{
  validate_config_for_mode(cfg, opts, harness_mode::negative_test);
  validate_negative_send_gates(cfg, opts);

  byte_buffer payload = build_ng_reset_message();
  dry_run_payload_summary("gnb-control", payload);
  std::printf("negative_test gnb_control gnb_id=%u gnb_id_bit_length=%u max_packet_count=%u min_interval_ms=%u\n",
              cfg.gnb.id,
              cfg.gnb.id_bit_length,
              cfg.negative_tests.max_packet_count,
              cfg.negative_tests.min_interval_ms);
  if (opts.dry_run) {
    std::printf("negative_test send_status=not_sent reason=dry_run\n");
    return;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(cfg.negative_tests.min_interval_ms));
  establish_setup_then_send_one(cfg, payload);
}

void ocudu::fbs::run_negative_malformed_stub(const injector_config& cfg, const runtime_options& opts)
{
  validate_config_for_mode(cfg, opts, harness_mode::negative_test);
  validate_negative_send_gates(cfg, opts);
  // TODO: Add only lab-approved malformed payload builders here; the stub intentionally cannot transmit.
  std::printf("negative_test malformed_stub status=not_implemented_send_disabled ");
  std::printf("note=\"Malformed payload generation is intentionally left as a gated lab extension TODO.\"\n");
  if (!opts.dry_run) {
    throw std::runtime_error("Malformed negative-test sending is a stub and cannot transmit");
  }
}
