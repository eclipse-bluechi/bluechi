# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import logging
import time
from typing import Dict

from bluechi_test.util import read_file
from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig

LOGGER = logging.getLogger(__name__)

node_one = "node-1"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):

    ctrl.create_file("/tmp", "system-monitor.py", read_file("python/system-monitor.py"))
    ctrl.copy_systemd_service("monitor.service", "systemd", os.path.join("/", "etc", "systemd", "system"))

    result, output = ctrl.exec_run("systemctl start monitor.service")
    if result != 0:
        raise Exception(f"Failed to start monitor service: {output}")

    # wait a bit so monitor is set up
    time.sleep(2)

    result, output = ctrl.exec_run("systemctl stop bluechi-controller")
    if result != 0:
        raise Exception(f"Failed to stop bluechi-controller: {output}")

    LOGGER.debug("Checking events processed by the system monitor...")
    expected_events = ["down"]
    while True:
        result, output = ctrl.exec_run("cat /tmp/events")
        if result != 0:
            raise Exception(f"Unexpected error while getting events file: {output}")

        events = output.split(",")
        LOGGER.info(f"Got monitored events: '{events}', comparing with expected '{expected_events}'")

        # output contains format like 'degraded,up,'
        # So -1 is used to take the additional element into account
        if (len(events) - 1) == len(expected_events):
            for i in range(len(expected_events)):
                assert events[i] == expected_events[i]
            break


def test_monitor_system_status(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    node_one_config = bluechi_node_default_config.deep_copy()
    node_one_config.node_name = node_one

    bluechi_ctrl_default_config.allowed_node_names = [node_one]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(node_one_config)

    bluechi_test.run(exec)
