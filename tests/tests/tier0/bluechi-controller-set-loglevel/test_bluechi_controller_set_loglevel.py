# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    service = "org.eclipse.bluechi"
    object = "/org/eclipse/bluechi"
    interface = "org.eclipse.bluechi.Controller"
    ctrl.exec_run("bluechictl set-loglevel INFO")
    _, output = ctrl.exec_run(f"busctl  get-property {service} {object} {interface} LogLevel")
    assert "INFO" in output
    ctrl.exec_run("bluechictl set-loglevel DEBUG")
    _, output = ctrl.exec_run(f"busctl  get-property {service} {object} {interface} LogLevel")
    assert "DEBUG" in output


def test_controller_setloglevel(
        bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.run(exec)
