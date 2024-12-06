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
    # Verify that the bluechi-agent is connected to bluechi-controller
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)

    # Stop both components
    ctrl.systemctl.stop_unit("bluechi-controller")
    nodes[NODE_FOO].systemctl.stop_unit("bluechi-agent")

    # Set UseTCP=False and overwrite controller config
    ctrl_config = ctrl.config.deep_copy()
    ctrl_config.use_tcp = False
    ctrl.create_file(
        ctrl_config.get_confd_dir(),
        ctrl_config.file_name,
        ctrl_config.serialize(),
    )

    # Start both components
    ctrl.systemctl.start_unit("bluechi-controller")
    ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")
    nodes[NODE_FOO].systemctl.start_unit("bluechi-agent")
    nodes[NODE_FOO].wait_for_unit_state_to_be("bluechi-agent", "active")

    # Copy the port check script and run it
    ctrl.copy_container_script("cmd-on-port.sh")
    res, output = ctrl.exec_run(f"bash /var/cmd-on-port.sh {ctrl_config.port}")
    assert (
        res == 2
    ), f"Expected bluechi-controller to not listen on port {ctrl_config.port}"

    _, output = ctrl.bluechictl.get_node_status(NODE_FOO, check_result=False)
    assert "offline" in output, f"Expected {NODE_FOO} to be offline"


def test_bluechi_controller_disable_tcp(
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
