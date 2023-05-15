# SPDX-License-Identifier: GPL-2.0-or-later

import re
import os
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.test import HirteTest
from hirte_test.util import filter_hirtectl_list_units


node_foo_name = "node-foo"
node_bar_name = "node-bar"


def verify_proxy_start(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    foo = nodes[node_foo_name]
    bar = nodes[node_bar_name]

    source_dir = "systemd"
    target_dir = os.path.join("/", "etc", "systemd", "system")
    requesting_service = "requesting.service"
    simple_service = "simple.service"

    foo.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    result, output = ctrl.exec_run(f"hirtectl start {node_foo_name} {requesting_service}")
    if result != 0:
        raise Exception(f"Failed to start requesting service on node foo: {output}")

    # verify the units have been loaded
    result, output = ctrl.exec_run(f"hirtectl list-units {node_bar_name}")
    if result != 0:
        raise Exception(f"Failed to list units for node bar: {output}")
    _, state, _ = filter_hirtectl_list_units(output, simple_service)
    assert state == "active"

    result, output = ctrl.exec_run(f"hirtectl list-units {node_foo_name}")
    if result != 0:
        raise Exception(f"Failed to list units for node foo: {output}")
    _, state, _ = filter_hirtectl_list_units(output, requesting_service)
    assert state == "active"


@pytest.mark.timeout(10)
def test_proxy_service_start(
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

    hirte_test.run(verify_proxy_start)
