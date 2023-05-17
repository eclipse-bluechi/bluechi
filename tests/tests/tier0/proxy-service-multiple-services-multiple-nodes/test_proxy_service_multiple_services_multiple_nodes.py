# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.test import HirteTest
from hirte_test.util import assemble_hirte_dep_service_name, assemble_hirte_proxy_service_name


node_foo1_name = "node-foo-1"
node_foo2_name = "node-foo-2"
node_bar_name = "node-bar"

requesting_service = "requesting.service"
simple_service = "simple.service"


def exec(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    foo1 = nodes[node_foo1_name]
    foo2 = nodes[node_foo2_name]
    bar = nodes[node_bar_name]

    source_dir = "systemd"
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo1.copy_systemd_service(requesting_service, source_dir, target_dir)
    foo2.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    hirte_dep_service = assemble_hirte_dep_service_name(simple_service)
    hirte_proxy_service = assemble_hirte_proxy_service_name(node_bar_name, simple_service)

    ctrl.start_unit(node_foo1_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "active")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "active")
    assert foo1.wait_for_unit_state_to_be(hirte_proxy_service, "active")

    ctrl.start_unit(node_foo2_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "active")
    assert foo2.wait_for_unit_state_to_be(requesting_service, "active")
    assert foo2.wait_for_unit_state_to_be(hirte_proxy_service, "active")

    ctrl.stop_unit(node_foo1_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "active")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(hirte_proxy_service, "inactive")

    ctrl.stop_unit(node_foo2_name, requesting_service)
    assert bar.wait_for_unit_state_to_be(simple_service, "inactive")
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(requesting_service, "inactive")
    assert foo1.wait_for_unit_state_to_be(hirte_proxy_service, "inactive")


@pytest.mark.timeout(15)
def test_proxy_service_multiple_services_multiple_nodes(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    node_foo1_cfg = hirte_node_default_config.deep_copy()
    node_foo1_cfg.node_name = node_foo1_name

    node_foo2_cfg = hirte_node_default_config.deep_copy()
    node_foo2_cfg.node_name = node_foo2_name

    node_bar_cfg = hirte_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    hirte_ctrl_default_config.allowed_node_names = [node_foo1_name, node_foo2_name, node_bar_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(node_foo1_cfg)
    hirte_test.add_hirte_node_config(node_foo2_cfg)
    hirte_test.add_hirte_node_config(node_bar_cfg)

    hirte_test.run(exec)
