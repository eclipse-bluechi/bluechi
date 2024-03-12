# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig


node_name_foo = "node-foo"
node_name_bar = "node-bar"

service_simple = "simple.service"
service_also_simple = "also-simple.service"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    nodes[node_name_foo].copy_systemd_service(service_simple)
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service_simple, "inactive")

    nodes[node_name_bar].copy_systemd_service(service_also_simple)
    assert nodes[node_name_bar].wait_for_unit_state_to_be(service_also_simple, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_wildcard_unit_changes(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiAgentConfig):

    config_node_foo = bluechi_node_default_config.deep_copy()
    config_node_bar = bluechi_node_default_config.deep_copy()

    config_node_foo.node_name = node_name_foo
    config_node_bar.node_name = node_name_bar

    bluechi_ctrl_default_config.allowed_node_names = [
        node_name_foo,
        node_name_bar,
    ]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(config_node_foo)
    bluechi_test.add_bluechi_agent_config(config_node_bar)

    bluechi_test.run(exec)
