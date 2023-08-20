# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


node_name = "node-foo"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))

    if result != 0:
        raise Exception(output)


@pytest.mark.timeout(10)
def test_monitor_node_disconnect(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    bluechi_node_default_config.node_name = node_name
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(bluechi_node_default_config)

    bluechi_test.run(exec)
