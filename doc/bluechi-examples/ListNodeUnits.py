#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0
#
# vim:sw=4:ts=4:et
from bluechi.api import Node

for unit in Node("my-node-name").list_units():
    # unit[name, description, ...]
    print(f"{unit[0]} - {unit[1]}")
