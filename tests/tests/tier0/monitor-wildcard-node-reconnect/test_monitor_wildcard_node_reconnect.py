# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


node_name_foo = "node-foo"
node_name_bar = "node-bar"
node_name_baz = "node-baz"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_wildcard_node_reconnect(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    config_node_foo = bluechi_node_default_config.deep_copy()
    config_node_bar = bluechi_node_default_config.deep_copy()
    config_node_baz = bluechi_node_default_config.deep_copy()

    config_node_foo.node_name = node_name_foo
    config_node_bar.node_name = node_name_bar
    config_node_baz.node_name = node_name_baz

    bluechi_ctrl_default_config.allowed_node_names = [
        node_name_foo,
        node_name_bar,
        node_name_baz,
    ]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(config_node_foo)
    bluechi_test.add_bluechi_node_config(config_node_bar)
    bluechi_test.add_bluechi_node_config(config_node_baz)

    bluechi_test.run(exec)
