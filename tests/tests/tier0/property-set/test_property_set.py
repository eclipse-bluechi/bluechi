# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleService
from bluechi_test.test import BluechiTest

node_name_foo = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    service = SimpleService()
    service.set_option(Section.Unit, Option.Description, "Just being true once")

    nodes[node_name_foo].install_systemd_service(service)
    assert nodes[node_name_foo].wait_for_unit_state_to_be(service.name, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "set_property.py"))
    if result != 0:
        raise Exception(output)


def test_property_set(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiAgentConfig):

    bluechi_node_default_config.node_name = node_name_foo
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(bluechi_node_default_config)

    bluechi_test.run(exec)
