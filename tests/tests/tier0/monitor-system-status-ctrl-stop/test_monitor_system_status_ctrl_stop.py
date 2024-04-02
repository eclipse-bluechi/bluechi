#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import time
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, Service
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)

node_one = "node-1"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    service = Service(name="monitor.service")
    service.set_option(Section.Unit, Option.Description, "Monitor BlueChi system")
    service.set_option(Section.Service, Option.Type, "simple")
    service.set_option(
        Section.Service, Option.ExecStart, "python3 /tmp/system-monitor.py"
    )
    service.set_option(Section.Install, Option.WantedBy, "multi-user.target")

    ctrl.create_file("/tmp", "system-monitor.py", read_file("python/system-monitor.py"))
    ctrl.install_systemd_service(service)

    result, output = ctrl.systemctl.start_unit(service.name)
    if result != 0:
        raise Exception(f"Failed to start monitor service: {output}")

    # wait a bit so monitor is set up
    time.sleep(2)

    result, output = ctrl.systemctl.stop_unit("bluechi-controller")
    if result != 0:
        raise Exception(f"Failed to stop bluechi-controller: {output}")

    LOGGER.debug("Checking events processed by the system monitor...")
    expected_events = ["down"]
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


def test_monitor_system_status(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_one_config = bluechi_node_default_config.deep_copy()
    node_one_config.node_name = node_one

    bluechi_ctrl_default_config.allowed_node_names = [node_one]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_one_config)

    bluechi_test.run(exec)
