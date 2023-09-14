#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0

from bluechi.api import Manager

for node in Manager().list_nodes():
    # node[name, obj_path, status]
    print(f"Node: {node[0]}, State: {node[2]}")
