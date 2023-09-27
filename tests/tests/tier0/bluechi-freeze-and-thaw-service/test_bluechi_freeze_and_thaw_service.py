# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.test import BluechiTest


node_foo_name = "node-foo"
simple_service = "simple.service"


def verify_service_start(foo: BluechiNodeContainer):
    assert foo.wait_for_unit_state_to_be(simple_service, "active")


def verify_service_freeze(foo: BluechiNodeContainer):
    output = foo.get_unit_freezer_state(simple_service)
    assert output == 'frozen'


def verify_service_thaw(foo: BluechiNodeContainer):
    output = foo.get_unit_freezer_state(simple_service)
    assert output == 'running'


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    foo = nodes[node_foo_name]

    source_dir = os.path.join(".", "systemd")
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(simple_service, source_dir, target_dir)
    assert foo.wait_for_unit_state_to_be(simple_service, "inactive")

    ctrl.start_unit(node_foo_name, simple_service)
    verify_service_start(foo)

    ctrl.freeze_unit(node_foo_name, simple_service)
    verify_service_freeze(foo)

    ctrl.thaw_unit(node_foo_name, simple_service)
    verify_service_thaw(foo)


def test_service_freeze_and_thaw(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(node_foo_cfg)

    bluechi_test.run(exec)
