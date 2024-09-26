#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    multi_user_string = "multi-user.target"
    graphical_string = "graphical.target"

    bc_return_code, bc_result = ctrl.bluechictl.get_default_target(NODE_FOO)
    systemd_return_code, systemd_result = node_foo.systemctl.get_default_target()
    assert systemd_result == bc_result and systemd_return_code == bc_return_code

    if systemd_result == multi_user_string:
        ctrl.bluechictl.set_default_target(NODE_FOO, graphical_string)
        _, systemd_result = node_foo.systemctl.get_default_target()
        assert systemd_result == graphical_string
    else:
        ctrl.bluechictl.set_default_target(NODE_FOO, multi_user_string)
        _, systemd_result = node_foo.systemctl.get_default_target()
        assert systemd_result == multi_user_string

    bc_return_code, bc_result = ctrl.bluechictl.get_default_target(NODE_FOO)
    assert systemd_result == bc_result and systemd_return_code == bc_return_code


def test_bluechictl_set_get_default_target(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
