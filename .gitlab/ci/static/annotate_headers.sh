#!/bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

set -euo pipefail

HDR_OCUDU="ocudu-default"
HDR_OCUDU_3GPP="ocudu-with-3gpp-notice"

COPYRIGHT_PREFIX="spdx-string-c"
COPYRIGHT_YEAR="2021-$(date +%Y)"
COPYRIGHT_HOLDER="Software Radio Systems Limited"

LICENSE="BSD-3-Clause-Open-MPI"

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

dirs_with_hdr_ocudu_3gpp=(
/apps/
/asn1/
/cu_cp/
/cu_up/
/du/
/e1ap/
/e2/
/f1ap/
/f1u/
/gtpu/
/mac/
/ngap/
/nrppa/
/nru/
/ntn/
/pdcp/
/phy/
/psup/
/ran/
/rlc/
/rohc/
/rrc/
/scheduler/
/sdap/
/security/
/xnap/
/ru/
/tests/
)

tmpfiles=()

files_all=$(mktemp)
tmpfiles+=("${files_all}")
files_cpp=$(mktemp)
tmpfiles+=("${files_cpp}")
files_generic=$(mktemp)
tmpfiles+=("${files_generic}")

files_cpp_ocudu=$(mktemp)
tmpfiles+=("${files_cpp_ocudu}")
files_cpp_ocudu_3gpp=$(mktemp)
tmpfiles+=("${files_cpp_ocudu_3gpp}")

files_generic_ocudu=$(mktemp)
tmpfiles+=("${files_generic_ocudu}")
files_generic_ocudu_3gpp=$(mktemp)
tmpfiles+=("${files_generic_ocudu_3gpp}")

match_ocudu_3gpp=$(mktemp)
tmpfiles+=("${match_ocudu_3gpp}")
printf "%s\n" "${dirs_with_hdr_ocudu_3gpp[@]}" > ${match_ocudu_3gpp}

EXCLUDES=(-name build -o -name .git -o -name external)

find . \
    \( -type d \( "${EXCLUDES[@]}" \) -prune \) -o \
    \( -type f \) > "${files_all}"

if $VERBOSE; then
  echo "Written to ${files_all}"
fi

match_cpp="\.(cpp|c|h)$"
grep    -E ${match_cpp} ${files_all} > ${files_cpp}
grep -v -E ${match_cpp} ${files_all} > ${files_generic}

grep    -E -f ${match_ocudu_3gpp} ${files_cpp} > ${files_cpp_ocudu_3gpp}
grep -v -E -f ${match_ocudu_3gpp} ${files_cpp} > ${files_cpp_ocudu}

grep    -E -f ${match_ocudu_3gpp} ${files_generic} > ${files_generic_ocudu_3gpp}
grep -v -E -f ${match_ocudu_3gpp} ${files_generic} > ${files_generic_ocudu}


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
REUSE_HDR_OCUDU=(
  "--template=${HDR_OCUDU}"
)
REUSE_HDR_OCUDU_3GPP=(
  "--template=${HDR_OCUDU_3GPP}"
)

XARGS_APP="xargs"
XARGS_BATCH_SIZE=100
XARGS_PROCS=$(nproc)
XARGS_BASE_ARGS=(
  "--max-lines=${XARGS_BATCH_SIZE}"
  "--max-procs=${XARGS_PROCS}"
)

"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_cpp_ocudu_3gpp}"     "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_CPP_ARGS}" "${REUSE_HDR_OCUDU_3GPP}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_cpp_ocudu}"          "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_CPP_ARGS}" "${REUSE_HDR_OCUDU}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_generic_ocudu_3gpp}" "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_GENERIC_ARGS}" "${REUSE_HDR_OCUDU_3GPP}"
"${XARGS_APP}" "${XARGS_BASE_ARGS[@]}" -a "${files_generic_ocudu}"      "${REUSE_APP}" "${REUSE_BASE_ARGS[@]}" "${REUSE_GENERIC_ARGS}" "${REUSE_HDR_OCUDU}"

rm ${tmpfiles[@]}
