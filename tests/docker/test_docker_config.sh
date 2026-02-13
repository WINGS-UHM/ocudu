#!/usr/bin/env bash
set -euo pipefail

# TDD validation script for docker deployment scenario changes
# Ported from srsRAN_Project PR #1458

PASS=0; FAIL=0
check() { if "$@"; then PASS=$((PASS+1)); else FAIL=$((FAIL+1)); echo "FAIL: $*"; fi }

# T1: .env default WS_URL should be gnb:8001
check grep -q '^WS_URL=gnb:8001' docker/.env

# T2: .env should NOT have hardcoded 172.19.1.3 as active value
if grep -q '^WS_URL=172\.19\.1\.3' docker/.env; then
    FAIL=$((FAIL+1)); echo "FAIL: .env still has hardcoded 172.19.1.3"
else
    PASS=$((PASS+1))
fi

# T3: .env should document all 4 deployment scenarios
check grep -q '\[A\]' docker/.env
check grep -q '\[B\]' docker/.env
check grep -q '\[C\]' docker/.env
check grep -q '\[D\]' docker/.env

# T4: docker-compose.ui.yml should include extra_hosts for host.docker.internal
check grep -q 'host.docker.internal:host-gateway' docker/docker-compose.ui.yml

# T5: README should include deployment scenario section
check grep -q 'Connecting to gNB' docker/README.md

# T6: README should include troubleshooting table
check grep -q 'Troubleshooting' docker/README.md

# T7: docker compose config should parse successfully (skip if docker not available)
if command -v docker &>/dev/null; then
    check docker compose -f docker/docker-compose.ui.yml config -q 2>/dev/null
else
    echo "SKIP: T7 (docker not available)"
fi

echo "Results: ${PASS} passed, ${FAIL} failed"
[ "$FAIL" -eq 0 ]
