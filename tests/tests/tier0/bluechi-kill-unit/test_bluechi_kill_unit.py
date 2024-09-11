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

    sleep_service = SimpleRemainingService(name="sleep.service")
    sleep_service.set_option(Section.Service, Option.ExecStart, "/bin/sleep 1000")
    node_foo.install_systemd_service(sleep_service)

    # killing all processes should succeed
    ctrl.bluechictl.start_unit(NODE_FOO, sleep_service.name)
    assert node_foo.wait_for_unit_state_to_be(sleep_service.name, "active")
    ctrl.bluechictl.kill_unit(NODE_FOO, sleep_service.name, whom="all", signal="9")
    assert node_foo.wait_for_unit_state_to_be(sleep_service.name, "failed")

    # there is only a main process when ExecStart= is run
    # so killing the control process should fail, but killing main process succeeds
    ctrl.bluechictl.start_unit(NODE_FOO, sleep_service.name)
    assert node_foo.wait_for_unit_state_to_be(sleep_service.name, "active")
    _, output = ctrl.bluechictl.kill_unit(
        NODE_FOO, sleep_service.name, whom="control", signal="9", check_result=False
    )
    assert "No control process to kill" in output
    assert node_foo.wait_for_unit_state_to_be(sleep_service.name, "active")
    ctrl.bluechictl.kill_unit(NODE_FOO, sleep_service.name, whom="main", signal="9")
    assert node_foo.wait_for_unit_state_to_be(sleep_service.name, "failed")


def test_bluechi_kill_unit(
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
