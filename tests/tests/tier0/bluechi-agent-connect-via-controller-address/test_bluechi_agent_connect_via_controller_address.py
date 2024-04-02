#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

NODE_FOO = "node-FOO"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    config_file_location = "/tmp"
    bluechi_agent_str = "bluechi-agent.service"
    invalid_conf_str = "Invalid-ControllerAddress.conf"
    invalid_controller_address = "Invalid-ControllerAddress.conf"
    service = "org.eclipse.bluechi"
    object = "/org/eclipse/bluechi"
    interface = "org.eclipse.bluechi.Controller"
    property = "Status"

    assert node_foo.wait_for_unit_state_to_be(bluechi_agent_str, "active", delay=2)
    assert (
        's "up"'
        == ctrl.exec_run(
            f"busctl get-property {service} {object} {interface} {property}"
        )[1]
    )

    content = read_file(os.path.join("config", invalid_conf_str))
    node_foo.create_file(config_file_location, invalid_conf_str, content)

    bc_agent = node_foo.load_systemd_service(
        directory="/usr/lib/systemd/system", name="bluechi-agent.service"
    )
    bc_agent.set_option(
        Section.Service,
        Option.ExecStart,
        bc_agent.get_option(Section.Service, Option.ExecStart)
        + " -c {}".format(
            os.path.join(config_file_location, invalid_controller_address)
        ),
    )
    node_foo.install_systemd_service(bc_agent, restart=True)
    assert node_foo.wait_for_unit_state_to_be(bc_agent.name, "active", delay=2)
    assert (
        's "down"'
        == ctrl.exec_run(
            f"busctl get-property {service} {object} {interface} {property}"
        )[1]
    )


def test_agent_invalid_configuration(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO
    node_foo_cfg.controller_address = (
        f"tcp:host={node_foo_cfg.controller_host},port={node_foo_cfg.controller_port}"
    )
    node_foo_cfg.controller_host = ""
    node_foo_cfg.controller_port = ""

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
