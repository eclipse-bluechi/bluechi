#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import time
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"
incorrect_name = "node-foo-foo"
incorrect_host = "111.111.111.111"
incorrect_port = "8425"
incorrect_heartbeat = "10000"


def get_address_str(host: str, port: str):
    return "tcp:host={},port={}".format(host, port)


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    # Create incorrect config file
    # If any of these configurations are applied - the node would not connect
    original_cfg = node_foo.config
    incorrect_cfg = original_cfg.deep_copy()
    incorrect_cfg.node_name = incorrect_name
    incorrect_cfg.controller_host = incorrect_host
    incorrect_cfg.controller_port = incorrect_port
    incorrect_cfg.heartbeat_interval = incorrect_heartbeat
    node_foo.create_file(
        incorrect_cfg.get_confd_dir(),
        incorrect_cfg.file_name,
        incorrect_cfg.serialize(),
    )

    # Restart agent with correct configurations passed via CLI arguments
    # The agent should start and connect correctly, as the CLI arguments are prioritized
    bc_agent = node_foo.load_systemd_service(
        directory="/usr/lib/systemd/system", name="bluechi-agent.service"
    )
    original_exec_start = bc_agent.get_option(Section.Service, Option.ExecStart)
    bc_agent.set_option(
        Section.Service,
        Option.ExecStart,
        original_exec_start
        + " -n {}".format(original_cfg.node_name)
        + " -H {}".format(original_cfg.controller_host)
        + " -p {}".format(original_cfg.controller_port)
        + " -i {}".format(original_cfg.heartbeat_interval),
    )
    LOGGER.debug(
        "Restarting agent with incorrect config file and correct config in CLI args"
    )
    node_foo.install_systemd_service(bc_agent, restart=True)
    assert node_foo.wait_for_unit_state_to_be(bc_agent.name, "active")

    # Check if node connected successfully
    time.sleep(1)
    LOGGER.debug("Veryfying node connection")
    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert result == 0

    # Verify the heartbeat is shorter than configured in agent.conf
    LOGGER.debug("Veryfying expected heartbeat interval")
    result, _ = ctrl.run_python(os.path.join("python", "verify_heartbeat.py"))
    assert result == 0

    # Reset configuration file and CLI args to test '-a' flag - connect via address
    incorrect_cfg = original_cfg.deep_copy()
    incorrect_cfg.controller_host = ""
    incorrect_cfg.controller_port = ""
    incorrect_cfg.controller_address = get_address_str(incorrect_host, incorrect_port)
    node_foo.create_file(
        incorrect_cfg.get_confd_dir(),
        incorrect_cfg.file_name,
        incorrect_cfg.serialize(),
    )
    bc_agent.set_option(
        Section.Service,
        Option.ExecStart,
        original_exec_start
        + " -a {}".format(
            get_address_str(original_cfg.controller_host, original_cfg.controller_port)
        ),
    )
    LOGGER.debug(
        "Restarting agent with incorrect config file and correct config in CLI args"
    )
    node_foo.install_systemd_service(bc_agent, restart=True)
    assert node_foo.wait_for_unit_state_to_be(bc_agent.name, "active")

    # Check if node connected successfully
    time.sleep(1)
    LOGGER.debug("Veryfying node connection")
    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert result == 0


def test_agent_cmd_options(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO
    node_foo_cfg.heartbeat_interval = "1000"

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
