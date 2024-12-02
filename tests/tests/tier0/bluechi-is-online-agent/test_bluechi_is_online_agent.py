#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def check_execs(node: BluechiAgentMachine):
    # check that bluechi-agent is online at the test startup
    result = node.bluechi_is_online.agent_is_online()
    assert result, f"bluechi-agent on node {node.name} should be online"

    # stop bluechi-agent
    node.systemctl.stop_unit("bluechi-agent")
    assert node.wait_for_unit_state_to_be("bluechi-agent", "inactive")

    # check that bluechi-agent is offline
    result = node.bluechi_is_online.agent_is_online()
    assert not result, f"bluechi-agent on node {node.name} should be offline"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    check_execs(node=node_foo)


def test_bluechi_is_online(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
