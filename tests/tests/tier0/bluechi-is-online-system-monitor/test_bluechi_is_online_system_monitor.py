#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
import logging
import threading
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest
from bluechi_test.util import Timeout, get_test_env_value_int

LOGGER = logging.getLogger(__name__)

AGENT_ONE = "agent-one"
AGENT_TWO = "agent-two"
SLEEP_DURATION = get_test_env_value_int("SLEEP_DURATION", 2)


class MonitorResult:
    def __init__(self):
        self.result = None
        self.output = ""


def monitor_command(ctrl: BluechiControllerMachine, monitor_result: MonitorResult):
    monitor_result.result, monitor_result.output = (
        ctrl.bluechi_is_online.monitor_system()
    )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    # Test 1: Both agents and controller are online
    LOGGER.debug("Testing with both agents and controller online.")
    monitor_result_test_one = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, monitor_result_test_one)
    )
    monitor_thread.start()
    try:
        with Timeout(SLEEP_DURATION, "Timeout while monitoring the system."):
            monitor_thread.join()
    except TimeoutError:
        LOGGER.debug(
            "Timeout reached while monitoring system. Attempting to terminate."
        )

    assert (
        monitor_result_test_one.result is None
    ), "Monitor command should not produce output when system is running."

    # Test 2: Agent-one offline
    LOGGER.debug("Stopping agent-one.")
    nodes[AGENT_ONE].systemctl.stop_unit("bluechi-agent")

    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    monitor_result_test_two = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, monitor_result_test_two)
    )
    monitor_thread.start()
    monitor_thread.join()

    assert (
        monitor_result_test_two.result is not None
        and monitor_result_test_two.output != ""
    ), "Monitor command should produce output when agent-one is offline."

    # Bring agent-one back online
    LOGGER.debug("Starting agent-one.")
    nodes[AGENT_ONE].systemctl.start_unit("bluechi-agent")

    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "active")

    # Test 3: Agent-two offline
    LOGGER.debug("Stopping agent-two.")
    nodes[AGENT_TWO].systemctl.stop_unit("bluechi-agent")

    assert nodes[AGENT_TWO].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    monitor_result_test_three = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, monitor_result_test_three)
    )
    monitor_thread.start()
    monitor_thread.join()

    assert (
        monitor_result_test_three.result is not None
        and monitor_result_test_three.output != ""
    ), "Monitor command should produce output when agent-two is offline."

    # Bring agent-two back online
    LOGGER.debug("Starting agent-two.")
    nodes[AGENT_TWO].systemctl.start_unit("bluechi-agent")

    # Test 4: Both agents offline
    LOGGER.debug("Stopping both agents.")
    nodes[AGENT_ONE].systemctl.stop_unit("bluechi-agent")
    nodes[AGENT_TWO].systemctl.stop_unit("bluechi-agent")

    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "inactive")
    assert nodes[AGENT_TWO].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    monitor_result_test_four = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, monitor_result_test_four)
    )
    monitor_thread.start()
    monitor_thread.join()

    assert (
        monitor_result_test_four.result is not None
        and monitor_result_test_four.output != ""
    ), "Monitor command should produce output when both agents are offline."

    # Bring both agents back online
    LOGGER.debug("Starting both agents.")
    nodes[AGENT_ONE].systemctl.start_unit("bluechi-agent")
    nodes[AGENT_TWO].systemctl.start_unit("bluechi-agent")

    # Test 5: Controller offline
    LOGGER.debug("Stopping the controller.")
    ctrl.systemctl.stop_unit("bluechi-controller")

    monitor_result_test_five = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(ctrl, monitor_result_test_five)
    )
    monitor_thread.start()
    monitor_thread.join()

    assert (
        monitor_result_test_five.result is not None
        and monitor_result_test_five.output != ""
    ), "Monitor command should produce output when the controller is offline."


def test_bluechi_is_online_system_monitor(
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
