# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import logging

from typing import Dict
from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)

NODE_FOO = "node-foo"
failed_status = "failed"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):

    node_foo = nodes[NODE_FOO]
    config_file_location = "/var/tmp"
    bluechi_agent_str = "bluechi-agent"
    invalid_conf_str = "config-files/invalid.conf"

    # Copying relevant config files into node container
    content = read_file(invalid_conf_str)
    node_foo.create_file(config_file_location, invalid_conf_str, content)

    LOGGER.debug("Setting invalid.conf as conf file for foo and checking if failed")
    node_foo.restart_with_config_file(os.path.join(config_file_location, invalid_conf_str), bluechi_agent_str)
    assert node_foo.wait_for_unit_state_to_be(bluechi_agent_str, failed_status)


def test_agent_config_c_option(
        bluechi_test: BlueChiTest,
        bluechi_node_default_config: BlueChiAgentConfig, bluechi_ctrl_default_config: BlueChiControllerConfig):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_test.add_bluechi_agent_machine_configs(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
