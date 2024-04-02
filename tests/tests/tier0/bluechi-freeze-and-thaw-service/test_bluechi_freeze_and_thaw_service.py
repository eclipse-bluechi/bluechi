#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import SleepingService
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    foo = nodes[node_foo_name]
    service = SleepingService()

    foo.install_systemd_service(service)
    assert foo.wait_for_unit_state_to_be(service.name, "inactive")

    ctrl.bluechictl.start_unit(node_foo_name, service.name)
    assert foo.wait_for_unit_state_to_be(service.name, "active")

    ctrl.bluechictl.freeze_unit(node_foo_name, service.name)
    assert "frozen" == foo.systemctl.get_unit_freezer_state(service.name)

    ctrl.bluechictl.thaw_unit(node_foo_name, service.name)
    assert "running" == foo.systemctl.get_unit_freezer_state(service.name)


def test_service_freeze_and_thaw(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
