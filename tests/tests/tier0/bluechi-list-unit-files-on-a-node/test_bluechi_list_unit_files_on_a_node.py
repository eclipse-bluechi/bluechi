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
    compare_lists,
    parse_bluechictl_list_output,
    parse_systemctl_list_output,
)
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_foo_name]

    all_res, all_out = ctrl.bluechictl.list_unit_files(node_name=node_foo_name)
    assert all_res == 0
    all_unit_files = parse_bluechictl_list_output(
        content=all_out,
        line_pattern=RegexPattern.BLUECHICTL_LIST_UNIT_FILES,
        item_class=SystemdUnitFile,
    )

    foo_res, foo_out = node_foo.systemctl.list_unit_files()
    assert foo_res == 0
    foo_unit_files = parse_systemctl_list_output(
        content=foo_out,
        line_pattern=RegexPattern.SYSTEMCTL_LIST_UNIT_FILES,
        item_class=SystemdUnitFile,
    )

    compare_lists(
        bc_items=all_unit_files[node_foo_name],
        sc_items=foo_unit_files,
        node_name=node_foo_name,
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
