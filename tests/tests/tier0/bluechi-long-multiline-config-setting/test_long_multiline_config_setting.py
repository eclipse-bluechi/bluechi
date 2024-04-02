#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def startup_verify(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")


def test_long_multiline_config_setting(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):
    config = bluechi_ctrl_default_config.deep_copy()
    for i in range(150):
        config.allowed_node_names.append(f"node-{i}")

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(startup_verify)
