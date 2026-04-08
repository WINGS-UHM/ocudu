#!/bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#
# Utilities used in container/runtime environments (e.g. curl, NTP client).
# Installs the OS-specific package list; takes no arguments.
#

set -e

install_docker_dependencies_debian_ubuntu() {
    local -a pkgs=(curl ntpdate)

    DEBIAN_FRONTEND=noninteractive apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${pkgs[@]}"
    apt-get clean && rm -rf /var/lib/apt/lists/*
}

install_docker_dependencies_arch() {
    local -a pkgs=(curl ntp)

    pacman -Syu --noconfirm "${pkgs[@]}"
    pacman -Scc --noconfirm
}

install_docker_dependencies_fedora() {
    local -a pkgs=(curl ntp)

    dnf -y install "${pkgs[@]}"
    dnf clean all
}

install_docker_dependencies_rhel() {
    local -a pkgs=(curl ntp)

    dnf -y install "${pkgs[@]}"
    dnf clean all
}

main() {
    if [ $# != 0 ]; then
        echo >&2 "Usage: $0"
        exit 1
    fi

    # shellcheck source=/dev/null
    . /etc/os-release

    echo "== Installing Docker/runtime helper packages =="

    case "$ID" in
        debian|ubuntu)
            install_docker_dependencies_debian_ubuntu
            ;;
        arch)
            install_docker_dependencies_arch
            ;;
        rhel)
            install_docker_dependencies_rhel
            ;;
        fedora)
            install_docker_dependencies_fedora
            ;;
        *)
            echo "OS $ID not supported"
            exit 1
            ;;
    esac
}

main "$@"
