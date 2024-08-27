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
node_bar_name = "node-bar"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    all_res, all_out = ctrl.bluechictl.list_unit_files()
    assert all_res == 0
    all_unit_files = parse_bluechictl_list_output(
        content=all_out,
        line_pattern=RegexPattern.BLUECHICTL_LIST_UNIT_FILES,
        item_class=SystemdUnitFile,
    )

    for node_name in nodes:
        node_res, node_out = nodes[node_name].systemctl.list_unit_files()
        assert node_res == 0
        node_unit_files = parse_systemctl_list_output(
            content=node_out,
            line_pattern=RegexPattern.SYSTEMCTL_LIST_UNIT_FILES,
            item_class=SystemdUnitFile,
        )
        compare_lists(
            bc_items=all_unit_files[node_name],
            sc_items=node_unit_files,
            node_name=node_name,
        )


def test_bluechi_list_unit_files_on_all_nodes(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name, node_bar_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)
    bluechi_test.add_bluechi_agent_config(node_bar_cfg)

    bluechi_test.run(exec)
