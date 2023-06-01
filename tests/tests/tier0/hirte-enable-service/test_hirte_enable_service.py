# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.test import HirteTest


node_foo_name = "node-foo"
simple_service = "simple.service"


def exec(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    foo = nodes[node_foo_name]

    source_dir = "systemd"
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(simple_service, source_dir, target_dir)

    _, output = foo.exec_run(f"systemctl is-enabled {simple_service}")
    if output != "disabled":
        raise Exception(f"Failed pre-check if unit {simple_service} is enabled: {output}")

    ctrl.enable_unit(node_foo_name, simple_service)

    _, output = foo.exec_run(f"systemctl is-enabled {simple_service}")
    if output != "enabled":
        raise Exception(f"Unit {simple_service} expected to be enabled, but got: {output}")


@pytest.mark.timeout(10)
def test_proxy_service_start(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    node_foo_cfg = hirte_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    hirte_ctrl_default_config.allowed_node_names = [node_foo_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(node_foo_cfg)

    hirte_test.run(exec)
