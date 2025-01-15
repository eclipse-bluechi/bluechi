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
from bluechi_test.util import Timeout, get_test_env_value_int

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"
SLEEP_DURATION = get_test_env_value_int("SLEEP_DURATION", 2)


class MonitorResult:
    def __init__(self):
        self.result = None
        self.output = ""


def monitor_command(node: BluechiAgentMachine, monitor_result: MonitorResult):
    monitor_result.result, monitor_result.output = (
        node.bluechi_is_online.monitor_agent()
    )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]

    # Test 1: Agent is running, no monitor output expected
    LOGGER.debug("Starting bluechi-agent.")
    monitor_result_test_one = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(node_foo, monitor_result_test_one)
    )

    monitor_thread.start()
    try:
        with Timeout(SLEEP_DURATION, "Timeout while monitoring agent"):
            monitor_thread.join()
    except TimeoutError:
        LOGGER.debug("Timeout reached while monitoring agent. Attempting to terminate.")

    assert (
        monitor_result_test_one.result is None
    ), "Monitor command should not produce output when agent is running."

    # Test 2: Stop bluechi-agent and verify monitoring detects the failure
    LOGGER.debug("Starting monitor thread before stopping the agent.")
    monitor_result_test_two = MonitorResult()
    monitor_thread = threading.Thread(
        target=monitor_command, args=(node_foo, monitor_result_test_two)
    )
    monitor_thread.start()
    time.sleep(SLEEP_DURATION)

    LOGGER.debug("Stopping the agent.")
    node_foo.systemctl.stop_unit("bluechi-agent")

    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")
    monitor_thread.join()
    assert (
        monitor_result_test_two.result is not None
        and monitor_result_test_two.output != ""
    ), "Monitor command should produce an output when agent is stopped."


def test_bluechi_is_online_agent_monitor(
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
