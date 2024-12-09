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


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    agent_one = nodes[AGENT_ONE]
    agent_two = nodes[AGENT_TWO]

    # Test 1: Both agents and controller are online
    LOGGER.debug("Testing with both agents and controller online.")
    assert ctrl.bluechi_is_online.system_is_online(), "Expected system to be online"

    # Test 2: Agent-one offline
    LOGGER.debug("Stopping agent-one.")
    agent_one.systemctl.stop_unit("bluechi-agent")

    assert (
        not ctrl.bluechi_is_online.system_is_online()
    ), "Expected system to report offline when agent-one is inactive"

    # Bring agent-one back online
    LOGGER.debug("Starting agent-one.")
    agent_one.systemctl.start_unit("bluechi-agent")

    # Test 3: Agent-two offline
    LOGGER.debug("Stopping agent-two.")
    agent_two.systemctl.stop_unit("bluechi-agent")

    assert (
        not ctrl.bluechi_is_online.system_is_online()
    ), "Expected system to report offline when agent-two is inactive"

    # Bring agent-two back online
    LOGGER.debug("Starting agent-two.")
    agent_two.systemctl.start_unit("bluechi-agent")

    # Test 4: Both agents are offline
    LOGGER.debug("Stopping both agents.")
    agent_one.systemctl.stop_unit("bluechi-agent")
    agent_two.systemctl.stop_unit("bluechi-agent")

    assert (
        not ctrl.bluechi_is_online.system_is_online()
    ), "Expected system to report offline when any agent is inactive"

    # Bring both agents back online
    LOGGER.debug("Starting both agents.")
    agent_one.systemctl.start_unit("bluechi-agent")
    agent_two.systemctl.start_unit("bluechi-agent")

    # Test 5: Controller offline
    LOGGER.debug("Stopping the controller.")
    ctrl.systemctl.stop_unit("bluechi-controller")

    assert not ctrl.bluechi_is_online.system_is_online(), (
        "Expected system to report offline when the controller is " "inactive"
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
