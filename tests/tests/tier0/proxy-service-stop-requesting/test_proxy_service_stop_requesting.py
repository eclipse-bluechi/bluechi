# SPDX-License-Identifier: GPL-2.0-or-later

import re
import os
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.test import HirteTest


node_foo_name = "node-foo"
node_bar_name = "node-bar"

requesting_service = "requesting.service"
simple_service = "simple.service"


def setup_and_start_proxy(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    foo = nodes[node_foo_name]
    bar = nodes[node_bar_name]

    source_dir = os.path.join(".", "systemd")
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    result, output = ctrl.exec_run(f"hirtectl start {node_foo_name} {requesting_service}")
    if result != 0:
        raise Exception(f"Failed to start requesting service on node foo: {output}")


def verify_proxies_running(ctrl: HirteControllerContainer):
    result, output = ctrl.exec_run(f"hirtectl list-units {node_bar_name} | grep {requesting_service}")
    if result != 0:
        raise Exception(f"Failed to list units for node bar: {output}")
    assert re.search("active", output) is not None

    result, output = ctrl.exec_run(f"hirtectl list-units {node_foo_name} | grep {simple_service}")
    if result != 0:
        raise Exception(f"Failed to list units for node foo: {output}")
    assert re.search("active", output) is not None


def stop_requesting_service(ctrl: HirteControllerContainer):
    result, output = ctrl.exec_run(f"hirtectl stop {node_foo_name} requesting.service")
    if result != 0:
        raise Exception(f"Failed to start requesting service on node foo: {output}")


def exec(ctrl: HirteControllerContainer, nodes: Dict[str, HirteNodeContainer]):
    setup_and_start_proxy(ctrl, nodes)
    verify_proxies_running(ctrl)
    stop_requesting_service(ctrl)

    # verify requesting service is stopped, but simple service is still running
    result, output = ctrl.exec_run(f"hirtectl list-units {node_bar_name} | grep {requesting_service}")
    if result != 0:
        raise Exception(f"Failed to list units for node bar: {output}")
    assert re.search("inactive", output) is not None

    result, output = ctrl.exec_run(f"hirtectl list-units {node_foo_name} | grep {simple_service}")
    if result != 0:
        raise Exception(f"Failed to list units for node foo: {output}")
    assert re.search("active", output) is not None


@pytest.mark.timeout(10)
def test_proxy_service_stop_requesting(
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
