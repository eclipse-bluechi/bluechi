#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)
NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]

    # Create a service which won't start due to nonexistent executable
    service = SimpleRemainingService()
    service.set_option(Section.Service, Option.ExecStart, "/s")

    node_foo.install_systemd_service(service)
    ctrl.bluechictl.start_unit(NODE_FOO, service.name)
    assert node_foo.wait_for_unit_state_to_be(service.name, "failed")

    node_foo.exec_run(
        f"sed -i '/ExecStart=/c\\ExecStart=/bin/true' /etc/systemd/system/{service.name}"
    )
    ctrl.bluechictl.daemon_reload_node(NODE_FOO)
    ctrl.bluechictl.start_unit(NODE_FOO, service.name)
    assert node_foo.wait_for_unit_state_to_be(service.name, "active")


def test_bluechi_deamon_reload(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
