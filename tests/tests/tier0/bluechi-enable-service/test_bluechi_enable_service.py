# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.test import BluechiTest


node_foo_name = "node-foo"
simple_service = "simple.service"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
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
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(node_foo_cfg)

    bluechi_test.run(exec)
