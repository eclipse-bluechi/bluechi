# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.fixtures import get_primary_ip
from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig


local_node_name = "local-foo"


def create_local_node_config() -> BlueChiAgentConfig:
    return BlueChiAgentConfig(
        file_name="agent.conf",
        controller_host=get_primary_ip(),
        controller_port='8420')


def verify_resolving_fqdn(ctrl: BlueChiControllerMachine, _: Dict[str, BlueChiAgentMachine]):
    # create config for local bluechi-agent and adding config to controller container
    local_node_cfg = create_local_node_config()
    local_node_cfg.node_name = local_node_name
    local_node_cfg.controller_host = "localhost"
    ctrl.create_file(local_node_cfg.get_confd_dir(), local_node_cfg.file_name, local_node_cfg.serialize())

    ctrl.systemctl_start_and_wait("bluechi-agent", 1)
    ctrl.wait_for_unit_state_to_be('bluechi-agent', 'active')

    result, output = ctrl.exec_run(f'bluechictl list-units {local_node_name}')
    assert result == 0


def test_agent_resolve_fqdn(bluechi_test: BlueChiTest, bluechi_ctrl_default_config: BlueChiControllerConfig):
    bluechi_ctrl_default_config.allowed_node_names = [local_node_name]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)

    bluechi_test.run(verify_resolving_fqdn)
