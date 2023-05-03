# SPDX-License-Identifier: GPL-2.0-or-later

from typing import Dict

from tests.lib.test import HirteTest
from tests.lib.container import HirteContainer
from tests.lib.config import HirteControllerConfig


def test_controller_startup(hirte_test: HirteTest, hirte_ctrl_default_config: HirteControllerConfig):
    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)

    def startup_verify(ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
        result, output = ctrl.exec_run('systemctl is-active hirte')

        assert result == 0
        assert output == 'active'

    hirte_test.run(startup_verify)
