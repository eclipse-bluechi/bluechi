# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig

node_name_foo = "node-foo"
node_name_bar = "node-bar"
service_simple = "simple.service"
service_also_simple = "also-simple.service"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    nodes[node_name_foo].copy_systemd_service(
        service_simple, "systemd", os.path.join("/", "etc", "systemd", "system"))
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service_simple, "inactive")

    nodes[node_name_foo].copy_systemd_service(
        service_also_simple, "systemd", os.path.join("/", "etc", "systemd", "system"))
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service_also_simple, "inactive")

    nodes[node_name_bar].copy_systemd_service(
        service_simple, "systemd", os.path.join("/", "etc", "systemd", "system"))
    assert nodes[node_name_bar].wait_for_unit_state_to_be(service_simple, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_specific_node_and_unit(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    node_foo_config = bluechi_node_default_config.deep_copy()
    node_bar_config = bluechi_node_default_config.deep_copy()

    node_foo_config.node_name = node_name_foo
    node_bar_config.node_name = node_name_bar
    bluechi_ctrl_default_config.allowed_node_names = [node_foo_config.node_name, node_bar_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(node_foo_config)
    bluechi_test.add_bluechi_node_config(node_bar_config)

    bluechi_test.run(exec)
