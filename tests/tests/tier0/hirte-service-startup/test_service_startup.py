# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from hirte_test.test import HirteTest
from hirte_test.container import HirteControllerContainer, HirteNodeContainer
from hirte_test.config import HirteControllerConfig


def startup_verify(ctrl: HirteControllerContainer, _: Dict[str, HirteNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active hirte')

    assert result == 0
    assert output == 'active'


@pytest.mark.timeout(5)
def test_controller_startup(hirte_test: HirteTest, hirte_ctrl_default_config: HirteControllerConfig):
    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)

    hirte_test.run(startup_verify)
