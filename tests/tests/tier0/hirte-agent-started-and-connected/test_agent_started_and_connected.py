# SPDX-License-Identifier: GPL-2.0-or-later

from typing import Dict

from tests.lib.test import HirteTest
from tests.lib.container import HirteContainer
from tests.lib.config import HirteControllerConfig, HirteNodeConfig


def test_agent_foo_startup(hirte_test: HirteTest, hirte_ctrl_default_config: HirteControllerConfig, hirte_node_default_config: HirteNodeConfig):
    hirte_node_default_config.node_name = "node-foo"
    hirte_ctrl_default_config.allowed_node_names = [hirte_node_default_config.node_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(hirte_node_default_config)

    def foo_startup_verify(ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
        result, output = ctrl.exec_run('systemctl is-active hirte')

        assert result == 0
        assert output == 'active'

        result, output = nodes["node-foo"].exec_run('systemctl is-active hirte-agent')

        assert result == 0
        assert output == 'active'

        # TODO: Add code to test that agent on node foo is successfully connected to hirte controller

    hirte_test.run(foo_startup_verify)
