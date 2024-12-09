#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
import logging
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

AGENT_ONE = "agent-one"
AGENT_TWO = "agent-two"


def check_exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    agent_one = nodes[AGENT_ONE]
    agent_two = nodes[AGENT_TWO]

    # Test 1: Both agents and controller are online
    LOGGER.debug("Testing with both agents and controller online.")
    result, output = ctrl.exec_run("bluechi-is-online system")
    assert result == 0, "Expected successful execution of bluechi-is-online system"
    assert not output, "Expected no output when all agents are online"

    # Test 2: Agent-one offline
    LOGGER.debug("Stopping agent-one.")
    agent_one.systemctl.stop_unit("bluechi-agent")
    assert agent_one.wait_for_unit_state_to_be("bluechi-agent", "inactive")

    result, output = ctrl.exec_run("bluechi-is-online system")
    assert (
        result != 0
    ), "Expected failure of bluechi-is-online system when agent-one is offline"
    assert (
        "org.eclipse.bluechi is offline" in output
    ), f"Expected output to indicate that the system is offline. Got: {output}"

    # Bring agent-one back online
    LOGGER.debug("Starting agent-one.")
    agent_one.systemctl.start_unit("bluechi-agent")
    assert agent_one.wait_for_unit_state_to_be("bluechi-agent", "active")

    # Test 3: Agent-two offline
    LOGGER.debug("Stopping agent-two.")
    agent_two.systemctl.stop_unit("bluechi-agent")
    assert agent_two.wait_for_unit_state_to_be("bluechi-agent", "inactive")

    result, output = ctrl.exec_run("bluechi-is-online system")
    assert (
        result != 0
    ), "Expected failure of bluechi-is-online system when agent-two is offline"
    assert (
        "org.eclipse.bluechi is offline" in output
    ), f"Expected output to indicate that the system is offline. Got: {output}"

    # Bring agent-two back online
    LOGGER.debug("Starting agent-two.")
    agent_two.systemctl.start_unit("bluechi-agent")
    assert agent_two.wait_for_unit_state_to_be("bluechi-agent", "active")

    # Test 4: Controller offline
    LOGGER.debug("Stopping the controller.")
    ctrl.systemctl.stop_unit("bluechi-controller")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "inactive")

    result, output = ctrl.exec_run("bluechi-is-online system")
    assert (
        result != 0
    ), "Expected failure of bluechi-is-online system when the controller is offline"
    assert (
        "org.eclipse.bluechi is offline" in output
    ), f"Expected output to indicate that the system is offline due to the controller. Got: {output}"

    # Restart the controller
    LOGGER.debug("Starting the controller.")
    ctrl.systemctl.start_unit("bluechi-controller")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    check_exec(
        ctrl=ctrl, nodes={AGENT_ONE: nodes[AGENT_ONE], AGENT_TWO: nodes[AGENT_TWO]}
    )


def test_bluechi_is_online_system(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):

    agent_one_cfg = bluechi_node_default_config.deep_copy()
    agent_one_cfg.node_name = AGENT_ONE

    agent_two_cfg = bluechi_node_default_config.deep_copy()
    agent_two_cfg.node_name = AGENT_TWO

    bluechi_ctrl_default_config.allowed_node_names = [AGENT_ONE, AGENT_TWO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(agent_one_cfg)
    bluechi_test.add_bluechi_agent_config(agent_two_cfg)

    bluechi_test.run(exec)
