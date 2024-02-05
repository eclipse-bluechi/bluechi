# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig


def foo_startup_verify(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):
    result, output = ctrl.exec_run('bluechictl status node-foo bluechi-agent.service')

    assert result == 0
    assert str(output).split('\n')[2].split('|')[2].strip() == 'active'

    # TODO: Add code to test that agent on node foo is successfully connected to bluechi controller


def test_agent_foo_startup(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    bluechi_node_default_config.node_name = "node-foo"
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(bluechi_node_default_config)

    bluechi_test.run(foo_startup_verify)
