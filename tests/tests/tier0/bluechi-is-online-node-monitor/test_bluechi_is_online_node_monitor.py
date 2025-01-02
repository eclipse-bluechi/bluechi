#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
import logging
import threading
import time
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"
AGENT_ONE = "agent-one"


def monitor_command(
    ctrl: BluechiControllerMachine, node_name: str, monitor_output: list
):
    """Run the node --monitor command and monitor output."""
    result = ctrl.bluechi_is_online.monitor_node(node_name)
    if not result:
        monitor_output.append(f"Monitoring node {node_name} failed.")


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    agent_one = nodes[AGENT_ONE]
    # Test 1: Agent and node are running, no monitor output expected
    LOGGER.debug("Starting NODE_FOO.")
    node_foo.systemctl.start_unit("bluechi-agent")
    # Verifying agent_one is online
    agent_one.bluechi_is_online.agent_is_online()

    monitor_output_test_one = []
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, NODE_FOO, monitor_output_test_one)
    )
    monitor_thread.start()
    time.sleep(2)
    assert (
        not monitor_output_test_one
    ), "Monitor command should not produce output when node is running."

    # Test 2: Stop NODE_FOO and verify monitoring detects the failure
    LOGGER.debug("Starting monitor thread before stopping NODE_FOO.")
    monitor_output_test_two = []
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, NODE_FOO, monitor_output_test_two)
    )
    monitor_thread.start()

    LOGGER.debug("Stopping NODE_FOO.")
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    assert (
        monitor_output_test_two
    ), "Monitor command should produce output when NODE_FOO is stopped."


def test_bluechi_is_online_node_monitor(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    agent_one_cfg = bluechi_node_default_config.deep_copy()
    agent_one_cfg.node_name = AGENT_ONE

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO, AGENT_ONE]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)
    bluechi_test.add_bluechi_agent_config(agent_one_cfg)

    bluechi_test.run(exec)
