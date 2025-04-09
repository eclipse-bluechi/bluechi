#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.constants import NODE_CTRL_NAME
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    ctrl.systemctl.stop_unit("bluechi-controller.service", check_result=False)
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller.service", "inactive")

    ctrl.systemctl.start_unit("bluechi-controller.socket")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller.socket", "active")

    ctrl.wait_for_unit_state_to_be("bluechi-controller.service", "active")

    os.environ["NODE_CTRL_NAME"] = NODE_CTRL_NAME
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_controller_socket_activation(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
