#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.systemd_lists import (
    RegexPattern,
    SystemdUnit,
    compare_lists,
    parse_bluechictl_list_output,
    parse_systemctl_list_output,
)
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"
node_bar_name = "node-bar"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    all_res, all_out = ctrl.bluechictl.list_units()
    assert all_res == 0
    all_units = parse_bluechictl_list_output(
        content=all_out,
        line_pattern=RegexPattern.BLUECHITL_LIST_UNITS,
        item_class=SystemdUnit,
    )

    for node_name in nodes:
        node_res, node_out = nodes[node_name].systemctl.list_units(all_units=True)
        assert node_res == 0
        node_units = parse_systemctl_list_output(
            content=node_out,
            line_pattern=RegexPattern.SYSTEMCTL_LIST_UNITS,
            item_class=SystemdUnit,
        )
        compare_lists(
            bc_items=all_units[node_name], sc_items=node_units, node_name=node_name
        )


def test_bluechi_nodes_statuses(
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
