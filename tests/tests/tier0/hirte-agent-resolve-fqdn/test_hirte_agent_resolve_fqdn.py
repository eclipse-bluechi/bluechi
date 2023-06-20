# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from hirte_test.fixtures import get_primary_ip
from hirte_test.test import HirteTest
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.config import HirteControllerConfig, HirteNodeConfig


local_node_name = "local-foo"


def create_local_node_config() -> HirteNodeConfig:
    return HirteNodeConfig(
        file_name="agent.conf",
        manager_host=get_primary_ip(),
        manager_port='8420')


def verify_resolving_fqdn(ctrl: HirteControllerContainer, _: Dict[str, HirteNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active hirte')
    assert result == 0
    assert output == 'active'

    # create config for local hirte-agent and adding config to controller container
    local_node_cfg = create_local_node_config()
    local_node_cfg.node_name = local_node_name
    local_node_cfg.manager_host = "localhost"
    ctrl.create_file(local_node_cfg.get_confd_dir(), local_node_cfg.file_name, local_node_cfg.serialize())

    ctrl.exec_run('systemctl start hirte-agent')
    result, output = ctrl.exec_run('systemctl is-active hirte-agent')
    assert result == 0
    assert output == 'active'

    result, output = ctrl.exec_run(f'hirtectl list-units {local_node_name}')
    assert result == 0


@pytest.mark.timeout(5)
def test_agent_resolve_fqdn(hirte_test: HirteTest, hirte_ctrl_default_config: HirteControllerConfig):
    hirte_ctrl_default_config.allowed_node_names = [local_node_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)

    hirte_test.run(verify_resolving_fqdn)
