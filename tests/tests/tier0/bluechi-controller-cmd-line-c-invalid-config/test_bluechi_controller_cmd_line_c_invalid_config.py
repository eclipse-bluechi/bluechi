#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)

failed_status = "failed"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    config_file_location = "/var/tmp"
    invalid_conf_str = "config-files/invalid.conf"

    # Copying relevant config files into the container
    content = read_file(invalid_conf_str)
    ctrl.create_file(config_file_location, invalid_conf_str, content)

    LOGGER.debug("Setting invalid.conf as conf file for ctrl and checking if failed")
    bc_controller = ctrl.load_systemd_service(
        directory="/usr/lib/systemd/system", name="bluechi-controller.service"
    )
    bc_controller.set_option(
        Section.Service,
        Option.ExecStart,
        bc_controller.get_option(Section.Service, Option.ExecStart)
        + " -c {}".format(os.path.join(config_file_location, invalid_conf_str)),
    )
    ctrl.install_systemd_service(bc_controller, restart=True)
    assert ctrl.wait_for_unit_state_to_be(bc_controller.name, failed_status)


def test_agent_config_c_option(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
