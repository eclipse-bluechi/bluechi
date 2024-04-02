#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict, List

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def check_output(
    machine: BluechiControllerMachine, cmd: str, out_list: List[str]
) -> bool:
    re, out = machine.exec_run(cmd)

    assert re != 0
    for item in out_list:
        assert item in out


def check_help_option(
    ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]
):
    executables = [
        "/usr/libexec/bluechi-controller",
        "/usr/libexec/bluechi-agent",
        "/usr/libexec/bluechi-proxy",
        "/usr/bin/bluechictl",
    ]

    for executable in executables:
        check_output(ctrl, f"{executable} -Q", ["Usage", "invalid option"])
        check_output(ctrl, f"{executable} --QWERTY", ["Usage", "unrecognized option"])


def test_help_option_provided(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(check_help_option)
