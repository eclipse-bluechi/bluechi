# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import time
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


node_name = "node-foo"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)

    ctrl.exec_run("systemctl stop bluechi")
    _, output = ctrl.exec_run('systemctl is-active bluechi')
    assert output == 'inactive'

    ctrl.exec_run("systemctl start bluechi")
    ctrl.wait_for_bluechi()
    # since the heartbeat (incl. a try to reconnect) is going to happen
    # every n milliseconds, lets wait a bit so this test is not becoming flaky
    time.sleep(1)

    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_node_disconnect(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    node_foo_config = bluechi_node_default_config.deep_copy()
    node_foo_config.node_name = node_name
    node_foo_config.heartbeat_interval = "500"

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(node_foo_config)

    bluechi_test.run(exec)
