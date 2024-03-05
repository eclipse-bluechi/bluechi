# SPDX-License-Identifier: LGPL-2.1-or-later
import logging
import os

from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    foo = nodes[NODE_FOO]
    target_dir = os.path.join("/", "etc", "systemd", "system")
    foo.copy_systemd_service("service-for-reload.service", "systemd", target_dir)
    foo.copy_systemd_service("simple.service", "systemd", target_dir)
    assert foo.wait_for_unit_state_to_be("simple.service", "inactive")
    assert foo.wait_for_unit_state_to_be("service-for-reload.service", "inactive")

    ctrl.bluechictl.start_unit(NODE_FOO, "service-for-reload.service")
    assert foo.wait_for_unit_state_to_be("simple.service", "inactive")
    assert foo.wait_for_unit_state_to_be("service-for-reload.service", "active")

    ctrl.bluechictl.reload_unit(NODE_FOO, "service-for-reload.service")
    assert foo.wait_for_unit_state_to_be("simple.service", "active")
    assert foo.wait_for_unit_state_to_be("service-for-reload.service", "active")


def test_bluechi_reload_unit_service(
        bluechi_test: BluechiTest,
        bluechi_node_default_config: BluechiAgentConfig, bluechi_ctrl_default_config: BluechiControllerConfig):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
