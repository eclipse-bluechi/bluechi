# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.test import BlueChiTest
from bluechi_test.util import assemble_bluechi_dep_service_name, assemble_bluechi_proxy_service_name


node_foo1_name = "node-foo-1"
node_foo2_name = "node-foo-2"
node_bar_name = "node-bar"

requesting_service = "requesting.service"
simple_service = "simple.service"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):
    foo1 = nodes[node_foo1_name]
    foo2 = nodes[node_foo2_name]
    bar = nodes[node_bar_name]

    source_dir = "systemd"
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo1.copy_systemd_service(requesting_service, source_dir, target_dir)
    foo2.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    assert foo1.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert foo2.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert bar.wait_for_unit_state_to_be(simple_service, "inactive")

    bluechi_dep_service = assemble_bluechi_dep_service_name(simple_service)
    bluechi_proxy_service = assemble_bluechi_proxy_service_name(node_bar_name, simple_service)

    ctrl.bluechictl.start_unit(node_foo1_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "active")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "active")
    assert foo1.wait_for_unit_state_to_be(bluechi_proxy_service, "active")

    ctrl.bluechictl.start_unit(node_foo2_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "active")
    assert foo2.wait_for_unit_state_to_be(requesting_service, "active")
    assert foo2.wait_for_unit_state_to_be(bluechi_proxy_service, "active")

    ctrl.bluechictl.stop_unit(node_foo1_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "active")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(bluechi_proxy_service, "inactive")

    ctrl.bluechictl.stop_unit(node_foo2_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "inactive")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(bluechi_proxy_service, "inactive")


def test_proxy_service_multiple_services_multiple_nodes(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    node_foo1_cfg = bluechi_node_default_config.deep_copy()
    node_foo1_cfg.node_name = node_foo1_name

    node_foo2_cfg = bluechi_node_default_config.deep_copy()
    node_foo2_cfg.node_name = node_foo2_name

    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo1_name, node_foo2_name, node_bar_name]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(node_foo1_cfg)
    bluechi_test.add_bluechi_agent_machine_configs(node_foo2_cfg)
    bluechi_test.add_bluechi_agent_machine_configs(node_bar_cfg)

    bluechi_test.run(exec)
