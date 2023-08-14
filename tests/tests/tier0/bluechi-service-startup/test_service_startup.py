# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig


def startup_verify(ctrl: BluechiControllerContainer, _: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active bluechi')

    assert result == 0
    assert output == 'active'


@pytest.mark.timeout(5)
def test_controller_startup(bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(startup_verify)
