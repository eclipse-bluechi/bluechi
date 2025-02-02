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

NODE_FOO = "node-foo"
IMMEDIATE_RETURN_TIMEOUT_MS = get_test_env_value_int("IMMEDIATE_RETURN_TIMEOUT", 1000)
WAIT_PARAM_VALUE_MS = get_test_env_value_int("WAIT_PARAM_VALUE", 5000)
SLEEP_DURATION = get_test_env_value_int("SLEEP_DURATION", 2)


class ResultFuture:
    def __init__(self):
        self.result = None
        self.output = ""


def check_node(
    bluechi_is_online: BluechiIsOnline,
    wait_time: int,
    future: ResultFuture,
):
    future.result = bluechi_is_online.node_is_online(
        NODE_FOO,
        wait_time,
    )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]

    # Test 1: Start agent and verify --wait option returns 0 immediately
    LOGGER.debug(
        f"Starting test number 1 - agent for node {NODE_FOO} should be online."
    )
    with Timeout(
        IMMEDIATE_RETURN_TIMEOUT_MS, "bluechi-is-online didn't return immediately"
    ):
        assert ctrl.bluechi_is_online.node_is_online(NODE_FOO, wait=WAIT_PARAM_VALUE_MS)

    # Test 2: Stop agent and verify node --wait returns 1 after wait time is expired
    node_foo.systemctl.stop_unit("bluechi-agent")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    LOGGER.debug(
        f"Starting test number 2 - agent for node {NODE_FOO} should remain offline."
    )

    start_time = time.time()
    result = ctrl.bluechi_is_online.node_is_online(NODE_FOO, wait=WAIT_PARAM_VALUE_MS)
    assert (
        not result
    ), f"Expected bluechi-is-online with node --wait={WAIT_PARAM_VALUE_MS} to return an error"
    assert (
        time.time() - start_time > WAIT_PARAM_VALUE_MS / 1000
    ), "Expected around 5 second for bluechi-is-online to exit"

    # Test 3: Stop agent, run 'bluechi-is-online node --wait', start agent and verify --wait returns 0 before the
    # wait time expires
    LOGGER.debug(
        "Starting test number 3, ensure agent is inactive before starting `bluechi-is-online`."
    )
    with Timeout(WAIT_PARAM_VALUE_MS, "Timeout during Test 3"):
        result_future_wait = ResultFuture()
        start_time = time.time()
        LOGGER.debug("Starting `bluechi-is-online` thread with wait time of 5 seconds.")
        check_thread_wait = threading.Thread(
            target=check_node,
            args=(
                ctrl.bluechi_is_online,
                WAIT_PARAM_VALUE_MS,
                result_future_wait,
            ),
        )
        check_thread_wait.start()
        time.sleep(SLEEP_DURATION)
        node_foo.systemctl.start_unit("bluechi-agent")
        assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "active")
        LOGGER.debug("Agent confirmed active after starting.")

        check_thread_wait.join()
        elapsed_time = time.time() - start_time
        LOGGER.debug(
            f"Test 3 result: {result_future_wait.result}, Elapsed time: {elapsed_time:.2f} seconds"
        )
        assert (
            result_future_wait.result
        ), f"Expected agent for node {NODE_FOO} to come online before wait expired"
        assert (
            elapsed_time < WAIT_PARAM_VALUE_MS / 1000
        ), "bluechi-is-online didn't finish before wait timeout"


def test_bluechi_is_online_node_wait(
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
