// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/adt/bounded_bitset.h"

int main()
{
  ocudu::bounded_bitset<5> a(4);

  a.flip(2);

  abort();
}
