# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.fixtures import get_primary_ip
from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


local_node_name = "local-foo"


def create_local_node_config() -> BluechiNodeConfig:
    return BluechiNodeConfig(
        file_name="agent.conf",
        manager_host=get_primary_ip(),
        manager_port='8420')


def verify_resolving_fqdn(ctrl: BluechiControllerContainer, _: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active bluechi-controller')
    assert result == 0
    assert output == 'active'

    # create config for local bluechi-agent and adding config to controller container
    local_node_cfg = create_local_node_config()
    local_node_cfg.node_name = local_node_name
    local_node_cfg.manager_host = "localhost"
    ctrl.create_file(local_node_cfg.get_confd_dir(), local_node_cfg.file_name, local_node_cfg.serialize())

    ctrl.systemctl_start_and_wait("bluechi-agent", 1)
    result, output = ctrl.exec_run('systemctl is-active bluechi-agent')
    assert result == 0
    assert output == 'active'

    result, output = ctrl.exec_run(f'bluechictl list-units {local_node_name}')
    assert result == 0


def test_agent_resolve_fqdn(bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):
    bluechi_ctrl_default_config.allowed_node_names = [local_node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(verify_resolving_fqdn)
