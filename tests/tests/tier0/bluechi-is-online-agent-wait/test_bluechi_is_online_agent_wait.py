#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import threading
import time
from typing import Dict

from bluechi_test.bluechi_is_online import BluechiIsOnline
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"
DEFAULT_TIMEOUT = int(os.getenv("BC_IS_ON_WAIT_TIMEOUT", 1000))
SHORT_TIMEOUT = int(os.getenv("BC_IS_ON_WAIT_TIMEOUT_SHORT", 5000))
LONG_TIMEOUT = int(os.getenv("BC_IS_ON_WAIT_TIMEOUT_LONG", 20000))
SLEEP_DURATION = int(os.getenv("SLEEP_DURATION", 2))


class ResultFuture:
    def __init__(self):
        self.result = None
        self.output = ""


def check_agent(
    bluechi_is_online: BluechiIsOnline,
    future: ResultFuture,
    description: str,
    wait_time: int = 0,
    expected_result: int = 0,
):
    result = bluechi_is_online.agent_is_online_with_wait(
        wait_time,
    )
    future.result = result


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]

    # Test 1: Start agent and verify --wait option returns 0 immediately
    node_foo.systemctl.start_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "active")
    LOGGER.debug("Starting test number 1 - agent should be online.")

    result_future_online = ResultFuture()
    start_time_immediately = time.time()
    check_thread_online = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_online,
            "Checking if agent is online",
            DEFAULT_TIMEOUT,
            0,
        ),
    )
    check_thread_online.start()
    check_thread_online.join()
    elapsed_time = time.time() - start_time_immediately
    assert (
        elapsed_time < 1 and result_future_online.result
    ), "Expected less than 1 second and the agent to be online"

    # Test 2: Stop agent and verify --wait returns 1 after wait time is expired.
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    LOGGER.debug("Starting test number 2 - agent should remain offline.")

    result_future_offline = ResultFuture()
    start_time_wait_5_sec = time.time()
    check_thread_offline = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_offline,
            "Checking if agent remains offline",
            SHORT_TIMEOUT,
            1,
        ),
    )
    check_thread_offline.start()
    check_thread_offline.join()
    elapsed_time_5_sec = time.time() - start_time_wait_5_sec
    assert 5 < elapsed_time_5_sec < 6, "--wait returned offline after 5 seconds"
    assert not result_future_offline.result, "Expected agent to remain offline"

    # Test 3: Stop agent, run 'bluechi-is-online agent --wait', start agent and verify --wait returns 0 before the
    # wait time expires
    LOGGER.debug(
        "Starting test number 3, stopping agent to ensure it is inactive before starting `bluechi-is-online` "
        "thread."
    )
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    LOGGER.debug("Agent confirmed inactive after stopping.")

    result_future_wait = ResultFuture()
    start_time = time.time()
    LOGGER.debug("Starting `bluechi-is-online` thread with wait time of 20 seconds.")
    check_thread_wait = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_wait,
            "Checking if agent comes online",
            LONG_TIMEOUT,
            0,
        ),
    )
    check_thread_wait.start()
    # Restart the agent after a delay to simulate it coming online during the --wait period
    time.sleep(SLEEP_DURATION)
    node_foo.systemctl.start_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "active")
    LOGGER.debug("Agent confirmed active after starting.")

    # Wait for the thread to complete
    check_thread_wait.join()
    elapsed_time = time.time() - start_time
    LOGGER.debug(
        f"Test 3 result: {result_future_wait.result}, Elapsed time: {elapsed_time:.2f} seconds"
    )
    assert (
        result_future_wait.result
    ), "Expected agent to come online before wait expired"
    assert (
        2 < elapsed_time < 3
    ), f"Command took too long to complete: {elapsed_time:.2f} seconds"


def test_bluechi_is_online_agent(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_bar_cfg)

    bluechi_test.run(exec)
