#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0

from bluechi.ext import Unit

Unit("my-node-name").disable_unit_files(["chronyd.service", "bluechi-agent.service"])
