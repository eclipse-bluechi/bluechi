#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    simple_svc = SimpleRemainingService()

    svc_for_reload = SimpleRemainingService(name="service-for-reload.service")
    svc_for_reload.set_option(
        Section.Service, Option.ExecReload, f"systemctl start {simple_svc.name}"
    )

    foo = nodes[NODE_FOO]
    foo.install_systemd_service(svc_for_reload)
    foo.install_systemd_service(simple_svc)
    assert foo.wait_for_unit_state_to_be(svc_for_reload.name, "inactive")
    assert foo.wait_for_unit_state_to_be(simple_svc.name, "inactive")

    ctrl.bluechictl.start_unit(NODE_FOO, svc_for_reload.name)
    assert foo.wait_for_unit_state_to_be(svc_for_reload.name, "active")
    assert foo.wait_for_unit_state_to_be(simple_svc.name, "inactive")

    ctrl.bluechictl.reload_unit(NODE_FOO, svc_for_reload.name)
    assert foo.wait_for_unit_state_to_be(svc_for_reload.name, "active")
    assert foo.wait_for_unit_state_to_be(simple_svc.name, "active")


def test_bluechi_reload_unit_service(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
