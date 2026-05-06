# NGAP Injector Research Harness

This app is a controlled NGAP test harness for authorized research inside an enclosed 5G SA lab testbed. It is not a scanner and it does not discover targets. It only operates against the AMF IP and local interface explicitly configured in a YAML file.

## Safety Constraints

- The configured AMF IP must appear in `allowlisted_amf_ips`.
- The configured sniff/send interface must appear in `allowlisted_interfaces`.
- The AMF IP must be private or lab scoped, such as RFC1918, loopback, link-local, CGNAT, or IPv6 ULA/link-local.
- A non-private AMF IP is refused unless `--lab-override` is paired with `--lab-override-confirm I_CONFIRM_CLOSED_LAB_USE_ONLY`.
- Hostname resolution is intentionally not supported for AMF or local gNB addresses.
- Send-capable modes default to dry-run. Actual sending requires `--confirm-send`.
- Negative-test sending also requires `--enable-negative-tests`.
- Negative tests send bounded traffic only. No target discovery, subnet probing, reconnect loops, or unbounded packet loops are implemented.
- UE identifiers must be manually configured or loaded from local passive-sniff context export.

## Build

Build the project normally with SCTP enabled. The app is under `apps/fbs/ngap_injector` and builds as `ngap_injector`.

```bash
cmake -B build -S .
cmake --build build --target ngap_injector
```

## Example Config

```yaml
local_gnb_ip: 10.10.0.2
amf_ip: 10.10.0.1
sctp_port: 38412
interface: n2-lab0

allowlisted_amf_ips:
  - 10.10.0.1
allowlisted_interfaces:
  - n2-lab0

mode: passive-sniff
pcap_path: /lab/captures/setup.pcapng

gnb:
  id: 411
  id_bit_length: 22
  ran_node_name: tstgnb01
  plmn: "00101"
  tac: 7
  sst: 1
  default_paging_drx: 256

ue:
  ran_ue_ngap_id: 7
  amf_ue_ngap_id: 9

negative_tests:
  max_packet_count: 1
  min_interval_ms: 1000
```

## Workflows

Passive sniff while a legitimate gNB/UE simulator runs:

```bash
ngap_injector passive-sniff --config config.yaml --export-context observed_context.json
```

Decode a known-good PCAP:

```bash
ngap_injector decode-pcap --config config.yaml --pcap setup.pcapng
```

Dry-run setup replay:

```bash
ngap_injector setup-replay --config config.yaml --pcap setup.pcapng --dry-run
```

Confirmed setup replay:

```bash
ngap_injector setup-replay --config config.yaml --pcap setup.pcapng --confirm-send
```

Dry-run constructed setup:

```bash
ngap_injector setup-construct --config config.yaml --dry-run
```

Dry-run controlled UE-context release validation:

```bash
ngap_injector negative-test ue-context-release --config config.yaml --context observed_context.json --dry-run
```

Confirmed controlled UE-context release validation:

```bash
ngap_injector negative-test ue-context-release --config config.yaml --context observed_context.json --enable-negative-tests --confirm-send
```

Dry-run gNB-side control validation:

```bash
ngap_injector negative-test gnb-control --config config.yaml --enable-negative-tests --dry-run
```

## Notes And Limitations

- PCAP/PCAPNG parsing extracts complete SCTP DATA chunks with NGAP PPID 60 from Ethernet, raw IP, and Linux cooked captures.
- Fragmented SCTP user messages are ignored for now rather than reassembled.
- NGAP decoding uses the repository ASN.1 decoder and currently reports high-value fields only.
- Setup construction uses configured gNB identity, PLMN, TAC, SST, and paging DRX. Full ASN.1 customization is a future TODO.
- Negative malformed-message generation is a stub by design and cannot transmit until a lab-approved payload path is added.
- For ZMQ UE simulation, sniff the configured local N2/testbed interface. The sniffer captures all local interface frames and filters only SCTP/NGAP payloads in memory.
