// UHM WINGS Fake Base Station Research

#include "CLI/CLI11.hpp"
#include "config.h"
#include "negative_tests.h"
#include "ngap_decoder.h"
#include "pcap_parser.h"
#include "sctp_transport.h"
#include "sniffer.h"
#include "ocudu/adt/span.h"
#include "ocudu/ocudulog/ocudulog.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

using namespace ocudu::fbs;

namespace {

struct command_selection {
  harness_mode mode = harness_mode::passive_sniff;
  std::string  negative_case;
};

void add_common_options(CLI::App& cmd, runtime_options& opts)
{
  cmd.add_option("--config", opts.config_path, "Path to NGAP injector YAML config")->required();
  cmd.add_flag("--lab-override", opts.lab_override, "Allow a non-private AMF IP only for an explicitly enclosed lab");
  cmd.add_option("--lab-override-confirm",
                 opts.lab_override_confirm,
                 std::string("Required confirmation string for --lab-override: ") + lab_override_confirmation);
}

void add_send_options(CLI::App& cmd, runtime_options& opts)
{
  cmd.add_flag("--dry-run", opts.dry_run_explicit, "Summarize without sending; this is the default for send modes");
  cmd.add_flag("--confirm-send", opts.confirm_send, "Actually transmit one bounded lab message");
}

void finalize_dry_run(runtime_options& opts)
{
  opts.dry_run = true;
  if (opts.confirm_send && !opts.dry_run_explicit) {
    opts.dry_run = false;
  }
}

std::string configured_pcap_path(const injector_config& cfg, const runtime_options& opts)
{
  if (!opts.pcap_path.empty()) {
    return opts.pcap_path;
  }
  if (cfg.pcap_path) {
    return *cfg.pcap_path;
  }
  throw std::runtime_error("No PCAP path provided by --pcap or config pcap_path");
}

void print_decoded_packets(const std::vector<ngap_pcap_packet>& packets)
{
  std::printf("pcap_ngap_packets count=%zu\n", packets.size());
  for (const auto& packet : packets) {
    const auto summary = decode_ngap_payload(packet.payload);
    std::printf("pcap_ngap index=%u packet=%u src=%s:%u dst=%s:%u %s\n",
                packet.ngap_index,
                packet.packet_index,
                packet.source_ip.c_str(),
                packet.source_port,
                packet.destination_ip.c_str(),
                packet.destination_port,
                format_summary(summary).c_str());
  }
}

const ngap_pcap_packet& select_setup_request_packet(const std::vector<ngap_pcap_packet>& packets,
                                                    const runtime_options& opts)
{
  for (const auto& packet : packets) {
    if (opts.selected_message_index_set && packet.ngap_index != opts.selected_message_index) {
      continue;
    }
    const auto summary = decode_ngap_payload(packet.payload);
    if (is_ng_setup_request(summary)) {
      return packet;
    }
  }
  if (opts.selected_message_index_set) {
    throw std::runtime_error("Selected PCAP message index is not an NG Setup Request");
  }
  throw std::runtime_error("No NG Setup Request found in PCAP");
}

byte_buffer byte_buffer_from_payload(const std::vector<uint8_t>& payload)
{
  byte_buffer buf{byte_buffer::fallback_allocation_tag{}};
  if (!payload.empty() && !buf.append(ocudu::span<const uint8_t>(payload.data(), payload.size()))) {
    throw std::runtime_error("Failed to allocate NGAP payload buffer");
  }
  return buf;
}

void run_decode_pcap(const runtime_options& opts)
{
  injector_config cfg = load_injector_config(opts.config_path);
  validate_config_for_mode(cfg, opts, harness_mode::decode_only);
  print_run_banner(cfg, opts, harness_mode::decode_only, false);

  const auto packets = read_ngap_packets_from_pcap(configured_pcap_path(cfg, opts));
  print_decoded_packets(packets);
}

void run_setup_replay(const runtime_options& opts)
{
  injector_config cfg = load_injector_config(opts.config_path);
  validate_config_for_mode(cfg, opts, harness_mode::setup_replay);
  print_run_banner(cfg, opts, harness_mode::setup_replay, !opts.dry_run);

  const auto packets = read_ngap_packets_from_pcap(configured_pcap_path(cfg, opts));
  print_decoded_packets(packets);
  const ngap_pcap_packet& selected = select_setup_request_packet(packets, opts);
  if (!packet_endpoints_are_configured_lab_ips(selected, cfg.local_gnb_ip, cfg.amf_ip, cfg.allowlisted_amf_ips)) {
    throw std::runtime_error("Refusing replay: selected PCAP packet endpoints are not configured lab IPs");
  }

  const byte_buffer setup_payload = byte_buffer_from_payload(selected.payload);
  std::printf("setup_replay selected_ngap_index=%u %s\n",
              selected.ngap_index,
              format_summary(decode_ngap_payload(setup_payload)).c_str());
  if (opts.dry_run) {
    std::printf("setup_replay send_status=not_sent reason=dry_run\n");
    return;
  }

  auto result = run_setup_exchange(cfg, setup_payload, 5000);
  std::printf("setup_replay result_state=%s response=\"%s\"\n",
              to_string(result.state),
              format_summary(result.response_summary).c_str());
}

void run_setup_construct(const runtime_options& opts)
{
  injector_config cfg = load_injector_config(opts.config_path);
  validate_config_for_mode(cfg, opts, harness_mode::setup_construct);
  print_run_banner(cfg, opts, harness_mode::setup_construct, !opts.dry_run);

  const byte_buffer setup_payload = build_ng_setup_request(cfg);
  std::printf("setup_construct payload_summary=\"%s\"\n", format_summary(decode_ngap_payload(setup_payload)).c_str());
  if (opts.dry_run) {
    std::printf("setup_construct send_status=not_sent reason=dry_run\n");
    return;
  }

  auto result = run_setup_exchange(cfg, setup_payload, 5000);
  std::printf("setup_construct result_state=%s response=\"%s\"\n",
              to_string(result.state),
              format_summary(result.response_summary).c_str());
}

void run_passive_sniff(const runtime_options& opts)
{
  injector_config cfg = load_injector_config(opts.config_path);
  validate_config_for_mode(cfg, opts, harness_mode::passive_sniff);
  print_run_banner(cfg, opts, harness_mode::passive_sniff, false);
  (void)run_passive_sniffer(cfg, opts.max_packets, opts.duration_seconds, opts.export_context_path);
}

void run_negative_case(const runtime_options& opts, const std::string& negative_case)
{
  injector_config cfg = load_injector_config(opts.config_path);
  print_run_banner(cfg, opts, harness_mode::negative_test, !opts.dry_run);

  if (negative_case == "ue-context-release") {
    run_negative_ue_context_release(cfg, opts);
    return;
  }
  if (negative_case == "gnb-control") {
    run_negative_gnb_control(cfg, opts);
    return;
  }
  if (negative_case == "malformed") {
    run_negative_malformed_stub(cfg, opts);
    return;
  }
  throw std::runtime_error("Unknown negative-test case: " + negative_case);
}

} // namespace

int main(int argc, char** argv)
{
  ocudulog::init();

  runtime_options  opts;
  command_selection selected;

  CLI::App app{"Controlled NGAP research injector/test harness for enclosed 5G SA labs"};
  app.require_subcommand(1);

  auto* passive = app.add_subcommand("passive-sniff", "Passively sniff local allowlisted N2 interface traffic");
  add_common_options(*passive, opts);
  passive->add_option("--export-context", opts.export_context_path, "Export observed UE context identifiers to JSON");
  passive->add_option("--max-packets", opts.max_packets, "Stop after this many decoded NGAP packets");
  passive->add_option("--duration-sec", opts.duration_seconds, "Stop after this many seconds");
  passive->callback([&selected]() { selected.mode = harness_mode::passive_sniff; });

  auto* decode = app.add_subcommand("decode-pcap", "Decode SCTP/NGAP payloads from a lab PCAP/PCAPNG");
  add_common_options(*decode, opts);
  decode->add_option("--pcap", opts.pcap_path, "PCAP/PCAPNG file to decode");
  decode->callback([&selected]() { selected.mode = harness_mode::decode_only; });

  auto* setup_replay = app.add_subcommand("setup-replay", "Replay an NG Setup Request from a lab PCAP");
  add_common_options(*setup_replay, opts);
  add_send_options(*setup_replay, opts);
  setup_replay->add_option("--pcap", opts.pcap_path, "PCAP/PCAPNG containing known-good NG setup messages");
  setup_replay
      ->add_option("--message-index", opts.selected_message_index, "Decoded NGAP packet index to replay")
      ->each([&opts](const std::string&) { opts.selected_message_index_set = true; });
  setup_replay->callback([&selected]() { selected.mode = harness_mode::setup_replay; });

  auto* setup_construct = app.add_subcommand("setup-construct", "Construct a minimal configured NG Setup Request");
  add_common_options(*setup_construct, opts);
  add_send_options(*setup_construct, opts);
  setup_construct->callback([&selected]() { selected.mode = harness_mode::setup_construct; });

  auto* negative = app.add_subcommand("negative-test", "Explicitly gated controlled negative tests");
  negative->require_subcommand(1);

  auto* ue_release = negative->add_subcommand("ue-context-release", "UE context release behavior validation");
  add_common_options(*ue_release, opts);
  add_send_options(*ue_release, opts);
  ue_release->add_flag("--enable-negative-tests", opts.enable_negative_tests, "Enable negative-test send path");
  ue_release->add_option("--context", opts.context_path, "Observed context JSON from passive-sniff");
  ue_release->add_option("--pcap", opts.pcap_path, "Optional lab PCAP containing a release-related NGAP message");
  ue_release->callback([&selected]() {
    selected.mode          = harness_mode::negative_test;
    selected.negative_case = "ue-context-release";
  });

  auto* gnb_control = negative->add_subcommand("gnb-control", "gNB-side NGAP control behavior validation");
  add_common_options(*gnb_control, opts);
  add_send_options(*gnb_control, opts);
  gnb_control->add_flag("--enable-negative-tests", opts.enable_negative_tests, "Enable negative-test send path");
  gnb_control->callback([&selected]() {
    selected.mode          = harness_mode::negative_test;
    selected.negative_case = "gnb-control";
  });

  auto* malformed = negative->add_subcommand("malformed", "Malformed/unexpected message handling stub");
  add_common_options(*malformed, opts);
  add_send_options(*malformed, opts);
  malformed->add_flag("--enable-negative-tests", opts.enable_negative_tests, "Enable negative-test send path");
  malformed->callback([&selected]() {
    selected.mode          = harness_mode::negative_test;
    selected.negative_case = "malformed";
  });

  try {
    app.parse(argc, argv);
    finalize_dry_run(opts);

    switch (selected.mode) {
      case harness_mode::passive_sniff:
        run_passive_sniff(opts);
        break;
      case harness_mode::decode_only:
        run_decode_pcap(opts);
        break;
      case harness_mode::setup_replay:
        run_setup_replay(opts);
        break;
      case harness_mode::setup_construct:
        run_setup_construct(opts);
        break;
      case harness_mode::negative_test:
        run_negative_case(opts, selected.negative_case);
        break;
    }
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_injector error: %s\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
