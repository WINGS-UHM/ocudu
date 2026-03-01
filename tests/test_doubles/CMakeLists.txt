# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


# Suppress "<ciso646> is deprecated in C++17" warnings caused by gtest when compiling with gcc 15.1.
ADD_CXX_COMPILER_FLAG_IF_AVAILABLE(-Wno-deprecated HAVE_NO_DEPRECATED)

add_subdirectory(mac)
add_subdirectory(rrc)
add_subdirectory(e1ap)
add_subdirectory(f1ap)
add_subdirectory(ngap)
add_subdirectory(nrppa)
add_subdirectory(scheduler)
add_subdirectory(pdcp)
add_subdirectory(du)
