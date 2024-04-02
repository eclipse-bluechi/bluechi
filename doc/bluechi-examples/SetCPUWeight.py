#!/usr/bin/env python
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from dasbus.typing import Variant

from bluechi.api import Node

Node("my-node-name").set_unit_properties(
    "ldconfig.service", False, [("CPUWeight", Variant("t", 18446744073709551615))]
)
