#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    result, output = ctrl.run_python(os.path.join("python", "start_transient.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_controller_start_transient_unit(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = "node-foo"

    bluechi_ctrl_default_config.allowed_node_names = ["node-foo"]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
