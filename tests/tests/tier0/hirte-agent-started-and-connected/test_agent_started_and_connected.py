# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from hirte_test.test import HirteTest
from hirte_test.container import HirteContainer
from hirte_test.config import HirteControllerConfig, HirteNodeConfig


def foo_startup_verify(ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
    result, output = ctrl.exec_run('systemctl is-active hirte')

    assert result == 0
    assert output == 'active'

    result, output = nodes["node-foo"].exec_run('systemctl is-active hirte-agent')

    assert result == 0
    assert output == 'active'

    # TODO: Add code to test that agent on node foo is successfully connected to hirte controller


@pytest.mark.timeout(5)
def test_agent_foo_startup(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    hirte_node_default_config.node_name = "node-foo"
    hirte_ctrl_default_config.allowed_node_names = [hirte_node_default_config.node_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(hirte_node_default_config)

    hirte_test.run(foo_startup_verify)
