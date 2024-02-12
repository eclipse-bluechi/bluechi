# SPDX-License-Identifier: LGPL-2.1-or-later
from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig


def exec(ctrl: BluechiControllerContainer, _: Dict[str, BluechiNodeContainer]):

    _, output = ctrl.exec_run("bluechictl set-loglevel INF")
    assert "Disconnect" not in output


def test_agent_config_c_option(
        bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
