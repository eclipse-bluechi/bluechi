#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)
NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_foo = nodes[NODE_FOO]

    failed_service_1 = SimpleRemainingService(name="simple1.service")
    failed_service_1.set_option(Section.Service, Option.ExecStart, "/s")
    failed_service_2 = SimpleRemainingService(name="simple2.service")
    failed_service_2.set_option(Section.Service, Option.ExecStart, "/s")
    failed_service_3 = SimpleRemainingService(name="simple3.service")
    failed_service_3.set_option(Section.Service, Option.ExecStart, "/s")
    sleeping_service = SimpleRemainingService(name="sleeping_service.service")
    sleeping_service.set_option(Section.Service, Option.ExecStart, "/bin/sleep 60")

    node_foo.install_systemd_service(failed_service_1)
    node_foo.install_systemd_service(failed_service_2)
    node_foo.install_systemd_service(failed_service_3)
    node_foo.install_systemd_service(sleeping_service)

    ctrl.bluechictl.start_unit(NODE_FOO, failed_service_1.name)
    ctrl.bluechictl.start_unit(NODE_FOO, failed_service_2.name)
    ctrl.bluechictl.start_unit(NODE_FOO, failed_service_3.name)
    ctrl.bluechictl.start_unit(NODE_FOO, sleeping_service.name)

    assert node_foo.wait_for_unit_state_to_be(failed_service_1.name, "failed")
    assert node_foo.wait_for_unit_state_to_be(failed_service_2.name, "failed")
    assert node_foo.wait_for_unit_state_to_be(failed_service_3.name, "failed")
    assert node_foo.wait_for_unit_state_to_be(sleeping_service.name, "active")

    units = [failed_service_1.name, failed_service_2.name]
    ctrl.bluechictl.reset_failed(node_name=NODE_FOO, units=units)

    assert node_foo.wait_for_unit_state_to_be(failed_service_1.name, "inactive")
    assert node_foo.wait_for_unit_state_to_be(failed_service_2.name, "inactive")
    assert node_foo.wait_for_unit_state_to_be(failed_service_3.name, "failed")
    assert node_foo.wait_for_unit_state_to_be(sleeping_service.name, "active")


def test_bluechi_reset_failed_units(
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
