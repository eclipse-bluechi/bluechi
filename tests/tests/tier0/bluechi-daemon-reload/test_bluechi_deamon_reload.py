# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import logging

from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig

LOGGER = logging.getLogger(__name__)
NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]
    service_file = "simple.service"
    service_path = os.path.join("/", "etc", "systemd", "system", service_file)

    node_foo.copy_systemd_service(service_file, "systemd",
                                  os.path.join("/", "etc", "systemd", "system"))
    ctrl.bluechictl.start_unit(NODE_FOO, service_file)
    assert node_foo.wait_for_unit_state_to_be(service_file, "failed")

    node_foo.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/bin/true' {service_path}")
    ctrl.bluechictl.daemon_reload_node(NODE_FOO)
    ctrl.bluechictl.start_unit(NODE_FOO, service_file)
    assert node_foo.wait_for_unit_state_to_be(service_file, "active")


def test_bluechi_deamon_reload(
        bluechi_test: BluechiTest,
        bluechi_node_default_config: BluechiAgentConfig, bluechi_ctrl_default_config: BluechiControllerConfig):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
