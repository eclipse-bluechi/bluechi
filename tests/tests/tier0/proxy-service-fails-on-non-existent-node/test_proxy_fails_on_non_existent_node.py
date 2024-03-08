# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"
requesting_service = "requesting.service"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    foo = nodes[node_foo_name]

    source_dir = os.path.join(".", "systemd")
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(requesting_service, source_dir, target_dir)
    assert foo.wait_for_unit_state_to_be(requesting_service, "inactive")

    ctrl.bluechictl.start_unit(node_foo_name, requesting_service)

    # Proxy fails to start since node-bar is offline, but since
    # requesting.service defines the dependency via Wants= its still
    # active - only the template service of the proxy is in failed state
    assert foo.wait_for_unit_state_to_be(requesting_service, "active")
    assert foo.wait_for_unit_state_to_be("bluechi-proxy@node-bar_simple.service", "failed")


def test_proxy_fails_on_non_existent_node(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiAgentConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
