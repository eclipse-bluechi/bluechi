#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.constants import NODE_CTRL_NAME
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")
    # local bluechi-agent is running, check if it is connected
    os.environ["NODE_CTRL_NAME"] = NODE_CTRL_NAME
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_and_agent_on_same_machine(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.run(exec)
