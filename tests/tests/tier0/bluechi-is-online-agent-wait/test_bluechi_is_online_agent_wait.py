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

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"


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
    result, output = bluechi_is_online.run(
        description,
        f"agent --wait {wait_time}",
        check_result=False,
        expected_result=expected_result,
    )
    future.result = result
    future.output = output


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]

    # Test 1: Start agent and verify --wait option returns 0 immediately
    node_foo.systemctl.start_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "active")
    LOGGER.debug("Starting test number 1 - agent should be online.")

    result_future_online = ResultFuture()
    check_thread_online = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_online,
            "Checking if agent is online",
            0,
            0,
        ),
    )
    check_thread_online.start()
    check_thread_online.join()
    assert result_future_online.result == 0, "Expected agent to be online"

    # Test 2: Stop agent and verify --wait returns 1 after wait time is expired.
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    LOGGER.debug("Starting test number 2 - agent should remain offline.")

    result_future_offline = ResultFuture()
    check_thread_offline = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_offline,
            "Checking if agent remains offline",
            5000,
            1,
        ),
    )
    check_thread_offline.start()
    check_thread_offline.join()
    assert result_future_offline.result != 0, "Expected agent to remain offline"

    # Test 3: Stop agent, run 'bluechi-is-online agent --wait', start agent and verify --wait returns 0 before the
    # wait time expires
    LOGGER.debug(
        "Starting test number 3, stopping agent to ensure it is inactive before starting `bluechi-is-online` "
        "thread."
    )
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    LOGGER.debug("Agent confirmed inactive after stopping.")

    # Add a short delay to ensure the "inactive" state is fully processed
    time.sleep(1)
    LOGGER.debug("Delay to ensure agent is fully offline before starting thread.")

    # Start the `bluechi-is-online` thread
    result_future_wait = ResultFuture()
    start_time = time.time()
    LOGGER.debug("Starting `bluechi-is-online` thread with wait time of 20 seconds.")
    check_thread_wait = threading.Thread(
        target=check_agent,
        args=(
            node_foo.bluechi_is_online,
            result_future_wait,
            "Checking if agent comes online",
            20000,
            0,
        ),
    )
    check_thread_wait.start()

    # Restart the agent after a delay to simulate it coming online during the --wait period
    time.sleep(5)
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
        result_future_wait.result == 0
    ), "Expected agent to come online before wait expired"
    assert (
        elapsed_time < 20
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
