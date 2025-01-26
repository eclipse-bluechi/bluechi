#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import threading
import time
from typing import Dict

from bluechi_test.bluechi_is_online import BluechiIsOnline
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest
from bluechi_test.util import Timeout, get_test_env_value_int

LOGGER = logging.getLogger(__name__)

AGENT_ONE = "agent-one"
AGENT_TWO = "agent-two"
IMMEDIATE_RETURN_TIMEOUT_MS = get_test_env_value_int("IMMEDIATE_RETURN_TIMEOUT", 1000)
WAIT_PARAM_VALUE_MS = get_test_env_value_int("WAIT_PARAM_VALUE", 5000)
SLEEP_DURATION = get_test_env_value_int("SLEEP_DURATION", 2)


class ResultFuture:
    def __init__(self):
        self.result = None
        self.output = ""


def check_system(
    bluechi_is_online: BluechiIsOnline,
    wait_time: int,
    future: ResultFuture,
):
    future.result = bluechi_is_online.system_is_online(
        wait_time,
    )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    # Test 1: Both agents and controller are online
    LOGGER.debug("Starting test number 1- both agents and controller online.")
    with Timeout(
        IMMEDIATE_RETURN_TIMEOUT_MS, "bluechi-is-online didn't return immediately"
    ):
        assert ctrl.bluechi_is_online.system_is_online(wait=WAIT_PARAM_VALUE_MS)

    # Test 2: Agent-one offline
    LOGGER.debug("Starting test number 2 - agent one should remain offline.")
    LOGGER.debug("Stopping agent-one.")
    nodes[AGENT_ONE].systemctl.stop_unit("bluechi-agent")
    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    start_time = time.time()
    result = ctrl.bluechi_is_online.system_is_online(wait=IMMEDIATE_RETURN_TIMEOUT_MS)
    assert (
        not result
    ), f"Expected bluechi-is-online with --wait={IMMEDIATE_RETURN_TIMEOUT_MS} to return an error"
    assert (
        time.time() - start_time > IMMEDIATE_RETURN_TIMEOUT_MS / 1000
    ), f"Expected around {IMMEDIATE_RETURN_TIMEOUT_MS} seconds for bluechi-is-online to exit"

    # Bring agent-one back online
    LOGGER.debug("Starting agent-one.")
    nodes[AGENT_ONE].systemctl.start_unit("bluechi-agent")
    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "active")

    # Test 3: Agent-two offline
    LOGGER.debug("Starting test number 3 - agent two should remain offline.")
    nodes[AGENT_TWO].systemctl.stop_unit("bluechi-agent")
    assert nodes[AGENT_TWO].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    start_time = time.time()
    result = ctrl.bluechi_is_online.system_is_online(wait=IMMEDIATE_RETURN_TIMEOUT_MS)
    assert (
        not result
    ), f"Expected bluechi-is-online with --wait={IMMEDIATE_RETURN_TIMEOUT_MS} to return an error"
    assert (
        time.time() - start_time > IMMEDIATE_RETURN_TIMEOUT_MS / 1000
    ), f"Expected around {IMMEDIATE_RETURN_TIMEOUT_MS} seconds for bluechi-is-online to exit"

    # Bring agent-two back online
    LOGGER.debug("Starting agent-two.")
    nodes[AGENT_TWO].systemctl.start_unit("bluechi-agent")

    # Test 4: Stop both agents, run bluechi-is-online with --wait, start both agents, and verify success
    LOGGER.debug("Starting test number 4 - stopping both agents.")
    nodes[AGENT_ONE].systemctl.stop_unit("bluechi-agent")
    nodes[AGENT_TWO].systemctl.stop_unit("bluechi-agent")

    assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "inactive")
    assert nodes[AGENT_TWO].wait_for_unit_state_to_be("bluechi-agent", "inactive")

    with Timeout(WAIT_PARAM_VALUE_MS, "Timeout during Test 4"):
        system_result_test_four = ResultFuture()
        start_time = time.time()
        LOGGER.debug("Starting `bluechi-is-online` thread with wait time of 5 seconds.")
        system_thread_wait = threading.Thread(
            target=check_system,
            args=(
                ctrl.bluechi_is_online,
                WAIT_PARAM_VALUE_MS,
                system_result_test_four,
            ),
        )
        system_thread_wait.start()
        time.sleep(SLEEP_DURATION)

        nodes[AGENT_ONE].systemctl.start_unit("bluechi-agent")
        assert nodes[AGENT_ONE].wait_for_unit_state_to_be("bluechi-agent", "active")
        nodes[AGENT_TWO].systemctl.start_unit("bluechi-agent")
        assert nodes[AGENT_TWO].wait_for_unit_state_to_be("bluechi-agent", "active")

        LOGGER.debug("Both agents confirmed active after starting.")

        system_thread_wait.join()
        elapsed_time = time.time() - start_time
        LOGGER.debug(
            f"Test 4 result: {system_result_test_four.result}, Elapsed time: {elapsed_time:.2f} seconds"
        )
        assert (
            system_result_test_four.result
        ), "Expected all agents to come online before wait expired"
        assert (
            elapsed_time < WAIT_PARAM_VALUE_MS / 1000
        ), "bluechi-is-online didn't finish before wait timeout"

    # Test 5: Controller offline
    LOGGER.debug("Starting test number 5 - stopping the controller.")
    ctrl.systemctl.stop_unit("bluechi-controller")

    system_result_test_five = ResultFuture()
    system_thread_wait = threading.Thread(
        target=check_system,
        args=(
            ctrl.bluechi_is_online,
            WAIT_PARAM_VALUE_MS,
            system_result_test_five,
        ),
    )
    system_thread_wait.start()
    system_thread_wait.join()
    assert (
        not system_result_test_five.result
    ), "System wait command should produce output when the controller is offline."


def test_bluechi_is_online_system_wait(
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
