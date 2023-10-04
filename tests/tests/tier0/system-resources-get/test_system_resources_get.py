# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import pytest
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.run_python(os.path.join("python", "get_system_resources.py"))
    if result != 0:
        raise Exception(output)


@pytest.mark.timeout(40)
def test_system_resources_get(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    bluechi_node_default_config.node_name = "node-foo"
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(bluechi_node_default_config)

    bluechi_test.run(exec)
