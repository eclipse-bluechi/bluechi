#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import time
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

node_name = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)

    ctrl.systemctl.stop_unit("bluechi-controller")
    ctrl.wait_for_unit_state_to_be("bluechi-controller", "inactive")

    # let's wait a bit so the agent has at least one failing reconnect attempt
    time.sleep(1)

    ctrl.systemctl.start_unit("bluechi-controller")
    ctrl.wait_for_bluechi_controller()
    # since the heartbeat (incl. a try to reconnect) is going to happen
    # every n milliseconds, let's wait a bit so this test is not becoming flaky
    time.sleep(1)

    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_node_disconnect(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_config = bluechi_node_default_config.deep_copy()
    node_foo_config.node_name = node_name
    node_foo_config.heartbeat_interval = "500"

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_config)

    bluechi_test.run(exec)
