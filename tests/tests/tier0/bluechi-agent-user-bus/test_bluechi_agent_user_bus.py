#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    bluechi_user = "bluechiuser"
    node_foo = nodes[NODE_FOO]
    node_foo.systemctl.stop_unit("bluechi-agent")
    ctrl.systemctl.stop_unit("bluechi-controller")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "inactive")

    node_foo.exec_run(f"useradd {bluechi_user} -u 55555")
    node_foo.exec_run("chmod -R 777 /var/tmp/bluechi-coverage")
    result, _ = node_foo.run_python(
        os.path.join("python", "start_agent_as_user.py"), bluechi_user
    )
    assert result == 0


def test_bluechi_agent_user_bus(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
