// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/adt/complex.h"

int main()
{
  ocudu::cbf16_t a;

  a = ocudu::to_cbf16({1.23, 4.56});
  abort();
}
