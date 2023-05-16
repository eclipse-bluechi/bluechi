# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.test import HirteTest
from hirte_test.util import assemble_hirte_dep_service_name, assemble_hirte_proxy_service_name


node_foo_name = "node-foo"
node_bar_name = "node-bar"

requesting_service = "requesting.service"
simple_service = "simple.service"


def verify_proxy_start(foo: HirteNodeContainer, bar: HirteNodeContainer):
    assert foo.wait_for_unit_state_to_be(requesting_service, "active")
    hirte_proxy_service = assemble_hirte_proxy_service_name(node_bar_name, simple_service)
    assert foo.wait_for_unit_state_to_be(hirte_proxy_service, "active")

    assert bar.wait_for_unit_state_to_be(simple_service, "active")
    hirte_dep_service = assemble_hirte_dep_service_name(simple_service)
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "active")


def verify_proxy_stop(foo: HirteNodeContainer, bar: HirteNodeContainer):
    assert foo.wait_for_unit_state_to_be(requesting_service, "active")
    hirte_proxy_service = assemble_hirte_proxy_service_name(node_bar_name, simple_service)
    assert foo.wait_for_unit_state_to_be(hirte_proxy_service, "inactive")

    assert bar.wait_for_unit_state_to_be(simple_service, "inactive")
    hirte_dep_service = assemble_hirte_dep_service_name(simple_service)
    assert bar.wait_for_unit_state_to_be(hirte_dep_service, "inactive")


def exec(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    foo = nodes[node_foo_name]
    bar = nodes[node_bar_name]

    source_dir = os.path.join(".", "systemd")
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    ctrl.start_unit(node_foo_name, requesting_service)
    verify_proxy_start(foo, bar)
    ctrl.stop_unit(node_bar_name, simple_service)
    verify_proxy_stop(foo, bar)


@pytest.mark.timeout(10)
def test_proxy_service_stop_target(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    node_foo_cfg = hirte_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    node_bar_cfg = hirte_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    hirte_ctrl_default_config.allowed_node_names = [node_foo_name, node_bar_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(node_foo_cfg)
    hirte_test.add_hirte_node_config(node_bar_cfg)

    hirte_test.run(exec)
