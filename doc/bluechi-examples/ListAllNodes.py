#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0

from bluechi.api import Controller

for node in Controller().list_nodes():
    # node[name, obj_path, status]
    print(f"Node: {node[0]}, State: {node[2]}")
