#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../.." && pwd)"
build_dir="${BUILD_DIR:-${repo_root}/build}"

cmake -S "${repo_root}" -B "${build_dir}"
cmake --build "${build_dir}" --target ngap_injector "$@"
