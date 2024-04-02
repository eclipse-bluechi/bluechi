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
    executables = [
        "/usr/libexec/bluechi-controller",
        "/usr/libexec/bluechi-agent",
        "/usr/libexec/bluechi-proxy",
        "/usr/bin/bluechictl",
    ]

    for executable in executables:
        s_re, s_out = ctrl.exec_run(f"{executable} -h")
        l_re, l_out = ctrl.exec_run(f"{executable} --help")

        assert s_re == 0
        assert l_re == 0
        assert "Usage" in s_out
        assert "Usage" in l_out
        assert s_out == l_out


def test_help_option_provided(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(check_help_option)
