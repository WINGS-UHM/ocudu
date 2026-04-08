#!/bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

set -e

install_dpdk_dependencies_debian_ubuntu() {
    local mode="${1:?}"
    local -a pkgs=()
    local -a pip_pkgs=()

    local -a build_pkgs=(
        curl apt-transport-https ca-certificates xz-utils
        python3-pip ninja-build g++ build-essential pkg-config libnuma-dev libfdt-dev pciutils
    )
    local -a extra_pkgs=(
        libatomic1 iproute2
    )
    local -a run_pkgs=(
        python3-pip libnuma-dev pciutils libfdt-dev libatomic1 iproute2
    )
    local -a pip_build_pkgs=(meson pyelftools)
    local -a pip_run_pkgs=(pyelftools)

    case "$mode" in
        all)
            pkgs+=( "${build_pkgs[@]}" "${extra_pkgs[@]}" )
            pip_pkgs+=( "${pip_build_pkgs[@]}" )
            ;;
        build)
            pkgs+=( "${build_pkgs[@]}" )
            pip_pkgs+=( "${pip_build_pkgs[@]}" )
            ;;
        run)
            pkgs+=( "${run_pkgs[@]}" )
            pip_pkgs+=( "${pip_run_pkgs[@]}" )
            ;;
        *)
            echo >&2 "Unsupported mode: $mode"
            exit 1
            ;;
    esac

    if ((${#pkgs[@]})); then
        DEBIAN_FRONTEND=noninteractive apt-get update
        DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${pkgs[@]}"
        apt-get autoremove -y && apt-get clean && rm -rf /var/lib/apt/lists/*
    fi

    if ((${#pip_pkgs[@]})); then
        pip3 install "${pip_pkgs[@]}" || pip3 install --break-system-packages "${pip_pkgs[@]}"
    fi
}

main() {

    if [ $# != 0 ] && [ $# != 1 ]; then
        echo >&2 "Illegal number of parameters"
        echo >&2 "Run like this: \"./install_dpdk_dependencies.sh [<mode>]\" where mode could be: build, run and all"
        echo >&2 "If mode is not specified, all dependencies will be installed"
        exit 1
    fi

    local mode="${1:-all}"

    # shellcheck source=/dev/null
    . /etc/os-release

    case "$ID" in
        debian|ubuntu)
            install_dpdk_dependencies_debian_ubuntu "$mode"
            ;;
        *)
            echo "OS $ID not supported"
            exit 1
            ;;
    esac

}

main "$@"
