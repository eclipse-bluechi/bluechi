# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import logging

from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)

failed_status = "failed"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    config_file_location = "/var/tmp"
    bluechi_controller_str = "bluechi-controller"
    invalid_conf_str = "config-files/invalid.conf"

    # Copying relevant config files into the container
    content = read_file(invalid_conf_str)
    ctrl.create_file(config_file_location, invalid_conf_str, content)

    LOGGER.debug("Setting invalid.conf as conf file for ctrl and checking if failed")
    ctrl.restart_with_config_file(os.path.join(config_file_location, invalid_conf_str), bluechi_controller_str)
    assert ctrl.wait_for_unit_state_to_be(bluechi_controller_str, failed_status)


def test_agent_config_c_option(
        bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
