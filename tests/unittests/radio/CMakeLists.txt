# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


if (ZEROMQ_FOUND AND ENABLE_ZEROMQ)
    add_subdirectory(zmq)
endif (ZEROMQ_FOUND AND ENABLE_ZEROMQ)
