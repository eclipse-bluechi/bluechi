# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.test import BlueChiTest


node_foo_name = "node-foo"
simple_service = "simple.service"


def verify_service_start(foo: BlueChiAgentMachine):
    assert foo.wait_for_unit_state_to_be(simple_service, "active")


def verify_service_freeze(foo: BlueChiAgentMachine):
    output = foo.systemctl.get_unit_freezer_state(simple_service)
    assert output == 'frozen'


def verify_service_thaw(foo: BlueChiAgentMachine):
    output = foo.systemctl.get_unit_freezer_state(simple_service)
    assert output == 'running'


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):
    foo = nodes[node_foo_name]

    source_dir = os.path.join(".", "systemd")
    target_dir = os.path.join("/", "etc", "systemd", "system")

    foo.copy_systemd_service(simple_service, source_dir, target_dir)
    assert foo.wait_for_unit_state_to_be(simple_service, "inactive")

    ctrl.bluechictl.start_unit(node_foo_name, simple_service)
    verify_service_start(foo)

    ctrl.bluechictl.freeze_unit(node_foo_name, simple_service)
    verify_service_freeze(foo)

    ctrl.bluechictl.thaw_unit(node_foo_name, simple_service)
    verify_service_thaw(foo)


def test_service_freeze_and_thaw(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(node_foo_cfg)

    bluechi_test.run(exec)
