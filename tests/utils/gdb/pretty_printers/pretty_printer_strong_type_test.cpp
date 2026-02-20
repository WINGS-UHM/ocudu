// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/adt/strong_type.h"
#include <cstdlib>

struct tag;

int main()
{
  ocudu::strong_type<int, tag, ocudu::strong_equality> a{42};

  std::abort();
}
