#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys
from collections import namedtuple

import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

EnabledServiceInfo = namedtuple(
    "EnabledServicesInfo", ["op_type", "symlink_file", "symlink_dest"]
)
EnableResponse = namedtuple(
    "EnableResponse", ["carries_install_info", "enabled_services_info"]
)

if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} node_name unit_name...")
    sys.exit(1)

node_name = sys.argv[1]

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

response = node.EnableUnitFiles(sys.argv[2:], False, False)
enable_response = EnableResponse(*response)
if enable_response.carries_install_info:
    print("The unit files included enablement information")
else:
    print("The unit files did not include any enablement information")

for e in enable_response.enabled_services_info:
    enabled_service_info = EnabledServiceInfo(*e)
    if enabled_service_info.op_type == "symlink":
        print(
            f"Created symlink {enabled_service_info.symlink_file} -> {enabled_service_info.symlink_dest}"
        )
    elif enabled_service_info.op_type == "unlink":
        print(f'Removed "{enabled_service_info.symlink_file}".')
