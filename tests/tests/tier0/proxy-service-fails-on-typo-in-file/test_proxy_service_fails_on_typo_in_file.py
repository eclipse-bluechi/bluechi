# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import pytest
from typing import Dict

from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.test import BluechiTest
from bluechi_test.util import assemble_bluechi_dep_service_name, assemble_bluechi_proxy_service_name


node_foo_name = "node-foo"
node_bar_name = "node-bar"

requesting_service = "requesting.service"
simple_service = "simple.service"


def verify_proxy_start_failed(foo: BluechiNodeContainer, bar: BluechiNodeContainer):
    assert foo.wait_for_unit_state_to_be(requesting_service, "active")
    bluechi_proxy_service = assemble_bluechi_proxy_service_name(node_bar_name, simple_service)
    assert foo.wait_for_unit_state_to_be(bluechi_proxy_service, "failed")

    assert bar.wait_for_unit_state_to_be(simple_service, "inactive")
    bluechi_dep_service = assemble_bluechi_dep_service_name(simple_service)
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "inactive")


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    foo = nodes[node_foo_name]
    bar = nodes[node_bar_name]

    source_dir = "systemd"
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(requesting_service, source_dir, target_dir)
    bar.copy_systemd_service(simple_service, source_dir, target_dir)

    ctrl.start_unit(node_foo_name, requesting_service)
    verify_proxy_start_failed(foo, bar)


@pytest.mark.timeout(20)
def test_proxy_service_fails_on_typo_in_file(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name, node_bar_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(node_foo_cfg)
    bluechi_test.add_bluechi_node_config(node_bar_cfg)

    bluechi_test.run(exec)
