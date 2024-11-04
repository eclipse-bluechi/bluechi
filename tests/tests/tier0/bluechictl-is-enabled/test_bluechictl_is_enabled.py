#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.systemd_lists import (
    RegexPattern,
    SystemdUnitFile,
    parse_bluechictl_list_output,
)
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def check_execs(
    ctrl: BluechiControllerMachine,
    node: BluechiAgentMachine,
    unit_name: str,
    check_output: bool = True,
):
    bc_res, bc_out = ctrl.bluechictl.is_enabled(
        node_name=node_foo_name, unit_name=unit_name
    )
    sc_res, sc_out = node.systemctl.is_enabled(unit_name=unit_name)

    assert bc_res == sc_res
    if check_output:
        assert bc_out == sc_out


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_foo_name]

    # Traversing over existing unit files is the easiest way to cover all existing enablement statuses, to improve
    # performance each status is checked only once
    checked_statuses = set()
    all_res, all_out = ctrl.bluechictl.list_unit_files(node_name=node_foo_name)
    assert all_res == 0
    all_unit_files = parse_bluechictl_list_output(
        content=all_out,
        line_pattern=RegexPattern.BLUECHICTL_LIST_UNIT_FILES,
        item_class=SystemdUnitFile,
    )
    for unit in all_unit_files[node_foo_name].values():
        if unit.state not in checked_statuses:
            checked_statuses.add(unit.state)
            check_execs(ctrl=ctrl, node=node_foo, unit_name=unit.key)

    # Error message from bluechictl is not completely the same as from systemctl for non-existent service
    check_execs(
        ctrl=ctrl, node=node_foo, unit_name="non-existent.service", check_output=False
    )


def test_bluechi_list_unit_files_on_a_node(
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
