#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.constants import NODE_CTRL_NAME
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest
from bluechi_test.util import (
    assemble_bluechi_dep_service_name,
    assemble_bluechi_proxy_service_name,
)

node_foo_name = NODE_CTRL_NAME
node_bar_name = "node-bar"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    bar = nodes[node_bar_name]

    simple_service = SimpleRemainingService()

    requesting_service = SimpleRemainingService(name="requesting.service")
    requesting_service.set_option(
        Section.Unit, Option.After, "bluechi-proxy@node-bar_simple.service"
    )
    requesting_service.set_option(
        Section.Unit, Option.Wants, "bluechi-proxy@node-bar_simple.service"
    )

    bluechi_proxy_service = assemble_bluechi_proxy_service_name(
        node_bar_name, simple_service.name
    )
    bluechi_dep_service = assemble_bluechi_dep_service_name(simple_service.name)

    ctrl.install_systemd_service(requesting_service)
    bar.install_systemd_service(simple_service)

    assert ctrl.wait_for_unit_state_to_be(requesting_service.name, "inactive")
    assert bar.wait_for_unit_state_to_be(simple_service.name, "inactive")

    ctrl.bluechictl.start_unit(node_foo_name, requesting_service.name)

    # Verify proxy start
    assert ctrl.wait_for_unit_state_to_be(requesting_service.name, "active")
    assert ctrl.wait_for_unit_state_to_be(bluechi_proxy_service, "active")
    assert bar.wait_for_unit_state_to_be(simple_service.name, "active")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "active")

    ctrl.bluechictl.stop_unit(node_bar_name, bluechi_dep_service)

    # Verify proxy stop
    assert ctrl.wait_for_unit_state_to_be(requesting_service.name, "active")
    assert ctrl.wait_for_unit_state_to_be(bluechi_proxy_service, "active")
    assert bar.wait_for_unit_state_to_be(simple_service.name, "active")
    assert bar.wait_for_unit_state_to_be(bluechi_dep_service, "inactive")


def test_proxy_service_stop_bluechi_dep(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    bluechi_ctrl_default_config.allowed_node_names = [node_bar_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_bar_cfg)

    bluechi_test.run(exec)
