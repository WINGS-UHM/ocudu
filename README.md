<!--
SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
SPDX-License-Identifier: BSD-3-Clause-Open-MPI
-->
# OCUDU – WINGS Lab ISAC-Enhanced Fork

This repository is an independent research-focused fork of the official 
[OCUDU project](https://gitlab.com/ocudu/ocudu).

While the upstream project provides core O-RAN DU functionality and standard telemetry support, this fork extends the codebase with additional instrumentation and visualization pipelines tailored for **ISAC (Integrated Sensing and Communications) research**.

Enhancements in this fork include:
- ISAC-specific metric extraction
- Extended WebSocket adapters
- Research-oriented visualization hooks
- Experimental sensing data integration

This fork is independently maintained by WINGS Lab (University of Hawaiʻi at Mānoa) and is not officially affiliated with or endorsed by the original OCUDU project maintainers.

We aim to periodically synchronize with upstream releases while preserving ISAC-specific research capabilities. 

<a href="https://gitlab.com/ocudu/ocudu/-/commits/dev" target="_blank">
  <img alt="Upstream base" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/WINGS-UHM/ocudu/main/.github/badges/upstream-base.json">
</a>


# The OCUDU Project

[![Pipeline](https://gitlab.com/ocudu/ocudu/badges/main/pipeline.svg)](https://gitlab.com/ocudu/ocudu/-/pipelines?scope=branches)
[![Documentation](https://img.shields.io/badge/docs-built-green?logo=docusaurus)](https://docs.ocudu.org)
![Code](https://img.shields.io/badge/code-C++17-informational)
![Build](https://img.shields.io/badge/build-CMake-informational)
[![License](https://img.shields.io/badge/license-BSD--3--Clause--Open--MPI-blue)](https://spdx.org/licenses/BSD-3-Clause-Open-MPI.html)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11899/badge)](https://www.bestpractices.dev/projects/11899)
[![Coverage](https://gitlab.com/ocudu/ocudu/badges/main/coverage.svg?min_good=98&min_acceptable=60)](https://docs.ocudu.org/coverage/index.html)

<img src="https://srs.io/wp-content/uploads/ocudu_color.png" alt="image" width="50%"/>

OCUDU is a permissively-licensed, open-source 5G (and beyond) CU/DU project designed for commercial deployment and broad industry adoption, as well as advanced research and development. OCUDU is a complete radio access network (RAN) solution compliant with 3GPP and O-RAN Alliance specifications and includes the full L1/2/3 stack with minimal external dependencies. OCUDU is govorned under the Linux Foundation.

This repository contains the RAN source code, architecture documentation, and tooling.

For general information, visit https://ocudu.org.

## Getting started

Build instructions and user guides are provided in the [OCUDU User manual](https://docs.ocudu.org/user_manual/installation/). We also host an extensive selection of tutorials.

## Documentation

Complete project documentation including developer guideline, configuration reference, tutorials, etc. is
hosted in [this](https://gitlab.com/ocudu/ocudu_docs) repo. The most recent version of the documentation
is available [here](https://docs.ocudu.org).

## Contributing

Our project welcomes contributions from any member of our community. To get started contributing,
please take a look at the [Developer Guide](https://docs.ocudu.org/dev_guide/contributing_guide/) with detailed instructions on how to best engange with us.

## Governance

The OCUDU project is governed by a framework of principles, values, policies and processes to help our community and constituents towards our shared goals.

The [Governance](https://gitlab.com/ocudu/Governance) repo is used by the Technical Steering Committee, which oversees governance of the project.

## License

This project is licensed under the BSD 3-Clause Open MPI variant License – see the [LICENSE](./LICENSE) file for details.
Portions of this software may implement 3GPP specifications, which may be subject to additional licensing requirements.