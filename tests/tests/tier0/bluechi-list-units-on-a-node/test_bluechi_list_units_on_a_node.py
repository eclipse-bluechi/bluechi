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


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_foo_name]

    bc_res, bc_out = ctrl.bluechictl.list_units(node_name=node_foo_name)
    assert bc_res == 0
    bc_units = parse_bluechictl_list_output(
        content=bc_out,
        line_pattern=RegexPattern.BLUECHITL_LIST_UNITS,
        item_class=SystemdUnit,
    )

    foo_res, foo_out = node_foo.systemctl.list_units(all_units=True)
    assert foo_res == 0
    foo_units = parse_systemctl_list_output(
        content=foo_out,
        line_pattern=RegexPattern.SYSTEMCTL_LIST_UNITS,
        item_class=SystemdUnit,
    )

    compare_lists(
        bc_items=bc_units[node_foo_name], sc_items=foo_units, node_name=node_foo_name
    )


def test_bluechi_list_units_on_a_node(
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
