#!/usr/bin/env python
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from bluechi.api import Controller

for node in Controller().list_nodes():
    # node[name, obj_path, status, peer_ip]
    print(f"Node: {node[0]}, State: {node[2]}")
