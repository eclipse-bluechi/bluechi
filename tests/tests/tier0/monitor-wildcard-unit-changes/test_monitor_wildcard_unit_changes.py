# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import pytest
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

    nodes[node_name_bar].copy_systemd_service(
        service_also_simple, "systemd", os.path.join("/", "etc", "systemd", "system"))
    assert nodes[node_name_bar].wait_for_unit_state_to_be(service_also_simple, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))
    if result != 0:
        raise Exception(output)


@pytest.mark.timeout(15)
def test_monitor_wildcard_unit_changes(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    config_node_foo = bluechi_node_default_config.deep_copy()
    config_node_bar = bluechi_node_default_config.deep_copy()

    config_node_foo.node_name = node_name_foo
    config_node_bar.node_name = node_name_bar

    bluechi_ctrl_default_config.allowed_node_names = [
        node_name_foo,
        node_name_bar,
    ]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(config_node_foo)
    bluechi_test.add_bluechi_node_config(config_node_bar)

    bluechi_test.run(exec)
