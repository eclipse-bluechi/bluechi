#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

expected_logtarget = "stderr-full"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    result, output = ctrl.run_python(
        os.path.join("python", "controller_has_logtarget.py")
    )
    if result != 0:
        raise Exception(
            f"Controller did not have expected log target '{expected_logtarget}: {output}"
        )


def test_bluechi_agent_get_logtarget(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    bluechi_ctrl_default_config.allowed_node_names = []
    bluechi_ctrl_default_config.log_target = expected_logtarget
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
