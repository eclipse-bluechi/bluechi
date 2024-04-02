#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):

    _, output = ctrl.bluechictl.set_log_level_on_node(
        NODE_FOO, "INF", check_result=False
    )
    print(output)
    assert b"Disconnect" not in output


def test_bluechi_agent_set_loglevel_invalid(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    bluechi_node_default_config.node_name = NODE_FOO
    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(bluechi_node_default_config)

    bluechi_test.run(exec)
