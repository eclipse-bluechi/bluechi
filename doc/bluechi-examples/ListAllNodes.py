#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0
#
# vim:sw=4:ts=4:et
from bluechi.api import Manager

for node in Manager().list_nodes():
    # node[name, obj_path, status]
    print(f"Node: {node[0]}, State: {node[2]}")
