#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import time
from typing import Dict, List

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, Service
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)

node_one = "node-1"
node_two = "node-2"
node_three = "node-3"
nodes = [node_one, node_two, node_three]


def stop_all_agents(nodes: Dict[str, BluechiControllerMachine]):
    LOGGER.debug("Stopping all agents...")
    for node_name, node in nodes.items():
        result, output = node.systemctl.stop_unit("bluechi-agent")
        if result != 0:
            raise Exception(
                f"Failed to stop bluechi-agent on node '{node_name}': {output}"
            )


def start_all_agents(nodes: Dict[str, BluechiControllerMachine]):
    LOGGER.debug("Starting all agents...")
    for node_name, node in nodes.items():
        result, output = node.systemctl.start_unit("bluechi-agent")
        if result != 0:
            raise Exception(
                f"Failed to stop bluechi-agent on node '{node_name}': {output}"
            )


def check_events(ctrl: BluechiControllerMachine, expected_events: List[str]):
    """
    Continuously poll the current content in /tmp/events. As soon as the file
    contains the same number of events as the expected_events, assert that the
    content is the same - the order is important.
    """

    LOGGER.debug("Checking events processed by the system monitor...")

    # In the worst case, this infinite loop will be cancelled by pytests timeout
    # Its left sooner when we get the expected number of events
    while True:
        result, output = ctrl.exec_run("cat /tmp/events")
        if result != 0:
            raise Exception(f"Unexpected error while getting events file: {output}")

        events = output.split(",")
        LOGGER.info(
            f"Got monitored events: '{events}', comparing with expected '{expected_events}'"
        )

        # output contains format like 'degraded,up,'
        # So -1 is used to take the additional element into account
        if (len(events) - 1) == len(expected_events):
            for i in range(len(expected_events)):
                assert events[i] == expected_events[i]
            break

        time.sleep(1)


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    service = Service(name="monitor.service")
    service.set_option(
        Section.Service, Option.ExecStart, "python3 /tmp/system-monitor.py"
    )
    service.set_option(Section.Unit, Option.Description, "Monitor BlueChi system")
    service.set_option(Section.Service, Option.Type, "simple")
    service.set_option(Section.Install, Option.WantedBy, "multi-user.target")

    ctrl.create_file("/tmp", "system-monitor.py", read_file("python/system-monitor.py"))
    ctrl.install_systemd_service(service)

    result, output = ctrl.systemctl.start_unit(service.name)
    if result != 0:
        raise Exception(f"Failed to start monitor service: {output}")

    # wait a bit so monitor is set up
    time.sleep(2)

    stop_all_agents(nodes)

    check_events(ctrl, ["degraded", "down"])

    start_all_agents(nodes)

    check_events(ctrl, ["degraded", "down", "degraded", "up"])


def test_monitor_system_status(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_one_config = bluechi_node_default_config.deep_copy()
    node_one_config.node_name = node_one
    node_two_config = bluechi_node_default_config.deep_copy()
    node_two_config.node_name = node_two
    node_three_config = bluechi_node_default_config.deep_copy()
    node_three_config.node_name = node_three

    bluechi_ctrl_default_config.allowed_node_names = nodes

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_one_config)
    bluechi_test.add_bluechi_agent_config(node_two_config)
    bluechi_test.add_bluechi_agent_config(node_three_config)

    bluechi_test.run(exec)
