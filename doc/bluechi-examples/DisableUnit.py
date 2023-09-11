#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0
#
# vim:sw=4:ts=4:et
from bluechi.ext import Unit

Unit("my-node-name").disable_unit_files(["chronyd.service", "bluechi-agent.service"])
