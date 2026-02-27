#!/bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

set -euo pipefail

HDR_OCUDU_DEFAULT="ocudu-default"
HDR_OCUDU_ALT="ocudu-alt"

COPYRIGHT_PREFIX="spdx-string-c"
COPYRIGHT_YEAR="2021-$(date +%Y)"
COPYRIGHT_HOLDER="Software Radio Systems Limited"

LICENSE="BSD-3-Clause-Open-MPI"

match_alt=".reuse/annotate-alt.txt"
match_exclude=".reuse/annotate-exclude.txt"

VERBOSE=false

# parse flags
while [[ $# -gt 0 ]]; do
  case "$1" in
    -v|--verbose)
      VERBOSE=true
      shift
      ;;
    *)
      break
      ;;
  esac
done

tmpfiles=()

files_all=$(mktemp)
tmpfiles+=("${files_all}")
files_filtered=$(mktemp)
tmpfiles+=("${files_filtered}")
files_cpp=$(mktemp)
tmpfiles+=("${files_cpp}")
files_generic=$(mktemp)
tmpfiles+=("${files_generic}")

files_cpp_default=$(mktemp)
tmpfiles+=("${files_cpp_default}")
files_cpp_alt=$(mktemp)
tmpfiles+=("${files_cpp_alt}")

files_generic_default=$(mktemp)
tmpfiles+=("${files_generic_default}")
files_generic_alt=$(mktemp)
tmpfiles+=("${files_generic_alt}")

find . -type f > "${files_all}"

if $VERBOSE; then
  echo "Written to ${files_all}"
fi

grep -v -E -f ${match_exclude} ${files_all} > ${files_filtered}

match_cpp="\.(cpp|c|h)$"
grep    -E ${match_cpp} ${files_filtered} > ${files_cpp}
grep -v -E ${match_cpp} ${files_filtered} > ${files_generic}

grep -v -E -f ${match_alt} ${files_cpp} > ${files_cpp_default}
grep    -E -f ${match_alt} ${files_cpp} > ${files_cpp_alt}

grep -v -E -f ${match_alt} ${files_generic} > ${files_generic_default}
grep    -E -f ${match_alt} ${files_generic} > ${files_generic_alt}


REUSE_APP="reuse"
REUSE_BASE_ARGS=(
  "annotate"
  "--copyright-prefix=${COPYRIGHT_PREFIX}"
  "--year=${COPYRIGHT_YEAR}"
  "--copyright=${COPYRIGHT_HOLDER}"
  "--license=${LICENSE}"
)
REUSE_CPP_ARGS=(
  "--style=cpp"
)
REUSE_GENERIC_ARGS=(
  "--skip-unrecognised"
)
REUSE_HDR_DEFAULT=(
  "--template=${HDR_OCUDU_DEFAULT}"
)
REUSE_HDR_ALT=(
  "--template=${HDR_OCUDU_ALT}"
)

XARGS_APP="xargs"
XARGS_BATCH_SIZE=100
XARGS_PROCS=$(nproc)
XARGS_BASE_ARGS=(
  "--max-lines=${XARGS_BATCH_SIZE}"
  "--max-procs=${XARGS_PROCS}"
)

"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_cpp_default}"     "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_CPP_ARGS}" "${REUSE_HDR_DEFAULT}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_cpp_alt}"         "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_CPP_ARGS}" "${REUSE_HDR_ALT}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_generic_default}" "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_GENERIC_ARGS}" "${REUSE_HDR_DEFAULT}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_generic_alt}"     "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_GENERIC_ARGS}" "${REUSE_HDR_ALT}"

rm ${tmpfiles[@]}
