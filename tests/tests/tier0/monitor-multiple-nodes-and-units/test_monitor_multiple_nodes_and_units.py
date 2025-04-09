#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.constants import NODE_CTRL_NAME
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import SimpleService
from bluechi_test.test import BluechiTest

node_name_foo = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    service1 = SimpleService()
    service2 = SimpleService(name="also-simple.service")
    nodes[node_name_foo].install_systemd_service(service1)
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service1.name, "inactive")

    nodes[node_name_foo].install_systemd_service(service2)
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service2.name, "inactive")

    ctrl.install_systemd_service(service1)
    assert ctrl.wait_for_unit_state_to_be(service1.name, "inactive")

    os.environ["NODE_CTRL_NAME"] = NODE_CTRL_NAME
    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_specific_node_and_unit(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_config = bluechi_node_default_config.deep_copy()

    node_foo_config.node_name = node_name_foo

    bluechi_ctrl_default_config.allowed_node_names = [node_name_foo]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_config)

    bluechi_test.run(exec)
