#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    service = SimpleRemainingService()
    service.set_option(Section.Service, Option.ExecStart, "/bin/true")

    node_foo.install_systemd_service(service)
    ctrl.bluechictl.daemon_reload_node(NODE_FOO)
    ctrl.bluechictl.start_unit(NODE_FOO, service.name)

    result, _ = ctrl.run_python(os.path.join("python", "bluechictl_status_watch.py"))
    assert result == 0


def test_bluechictl_status_watch_service(
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
