# SPDX-License-Identifier: LGPL-2.1-or-later
from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):

    _, output = ctrl.bluechictl.set_log_level_on_controller("INF", check_result=False)
    assert "Disconnect" not in output


def test_bluechi_controller_set_loglevel_invalid(
        bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.run(exec)
