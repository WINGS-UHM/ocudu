# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


ARG OS_VERSION=24.04

FROM ubuntu:$OS_VERSION AS builder
RUN DEBIAN_FRONTEND=noninteractive TZ=Europe/Madrid apt-get update && apt-get install -y --no-install-recommends \
    clang-format git colordiff apt-transport-https ca-certificates gawk yamllint \
    && apt-get autoremove && apt-get clean && rm -rf /var/lib/apt/lists/*

ADD hex2lowercase.sh include-guards-check.sh is-pristine-repo.sh run-clang-format-diff.sh update_headers.sh /usr/local/bin/
