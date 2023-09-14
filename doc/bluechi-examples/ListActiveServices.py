#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0

from bluechi.api import Manager

for unit in Manager().list_units():
    # unit[node, name, description, load_state, active_state, ...]
    if unit[4] == "active" and unit[1].endswith(".service"):
        print(f"Node: {unit[0]}, Unit: {unit[1]}")
