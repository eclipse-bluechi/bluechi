#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def check_help_option(
    ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]
):
    result, out = ctrl.exec_run("/usr/bin/bluechictl")
    assert result != 0
    assert "Usage" in out


def test_help_option_provided(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(check_help_option)
