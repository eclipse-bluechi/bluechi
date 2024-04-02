#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.bluechictl import LogLevel
from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    service = "org.eclipse.bluechi"
    object = "/org/eclipse/bluechi"
    interface = "org.eclipse.bluechi.Controller"
    ctrl.bluechictl.set_log_level_on_controller(LogLevel.INFO.value)
    _, output = ctrl.exec_run(
        f"busctl  get-property {service} {object} {interface} LogLevel"
    )
    assert "INFO" in output
    ctrl.bluechictl.set_log_level_on_controller(LogLevel.DEBUG.value)
    _, output = ctrl.exec_run(
        f"busctl  get-property {service} {object} {interface} LogLevel"
    )
    assert "DEBUG" in output


def test_controller_set_log_level(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.run(exec)
