# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from hirte_test.test import HirteTest
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.config import HirteControllerConfig, HirteNodeConfig

node_foo_name = "node-foobar"


def startup_verify(ctrl: HirteControllerContainer, _: Dict[str, HirteNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active hirte')

    assert result == 0
    assert output == 'active'


@pytest.mark.timeout(5)
def test_controller_startup(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    node_foo_cfg = hirte_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name
    hirte_ctrl_default_config.allowed_node_names = [node_foo_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(node_foo_cfg)

    hirte_test.run(startup_verify)
