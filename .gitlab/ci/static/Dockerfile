#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

ARG OS_VERSION=24.04

FROM ubuntu:$OS_VERSION AS builder
RUN DEBIAN_FRONTEND=noninteractive TZ=Europe/Madrid apt-get update && apt-get install -y --no-install-recommends \
    clang-format git colordiff apt-transport-https ca-certificates gawk yamllint \
    && apt-get autoremove && apt-get clean && rm -rf /var/lib/apt/lists/*

ADD hex2lowercase.sh include-guards-check.sh is-pristine-repo.sh run-clang-format-diff.sh update_headers.sh /usr/local/bin/
