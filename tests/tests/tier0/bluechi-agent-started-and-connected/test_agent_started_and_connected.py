# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


CONTROLLER_NAME = "bluechi-controller"
NODE_FOO = "node-foo"


def create_local_node_config(ip_controller: str) -> BluechiNodeConfig:
    return BluechiNodeConfig(
        file_name="agent.conf",
        manager_host=ip_controller,
        manager_port='8420')


def foo_startup_verify(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    result, output = ctrl.exec_run('systemctl is-active bluechi')
    assert result == 0
    assert output == 'active'

    print("======== DOUG list containers =======")
    ctrl.list_containers()
    print("======== DOUG list containers =======")

    IP_CONTROLLER = ctrl.get_ip()
    print("======== DOUG IP_CONTROLLER =======")
    print(IP_CONTROLLER)
    print("======== DOUG IP_CONTROLLER =======")

    local_node_cfg = create_local_node_config(IP_CONTROLLER)
    local_node_cfg.node_name = CONTROLLER_NAME
    local_node_cfg.manager_host = IP_CONTROLLER
    ctrl.create_file(local_node_cfg.get_confd_dir(), local_node_cfg.file_name, local_node_cfg.serialize())

    result = None
    result = nodes[NODE_FOO].set_config(
            "ManagerHost",
            IP_CONTROLLER,
            "/etc/bluechi/agent.conf.d/agent.conf"
    )

    result, output = nodes[NODE_FOO].exec_run("cat /etc/bluechi/agent.conf.d/agent.conf")
    print("======== DOUG output node_foo agent.conf =======")
    print(output)
    print(result)
    print("======== DOUG output node_foo agent.conf =======")
    #assert result == 0

    result, output = nodes[NODE_FOO].exec_run('systemctl restart bluechi-agent')
    print("======== DOUG systemctl restart =======")
    print(output)
    print(result)
    print("======== DOUG systemctl restart =======")

    #assert result == 0


    result, output = ctrl.exec_run('systemctl status bluechi-agent')
    print("======== DOUG status =======")
    print(output)
    print(result)
    print("======== DOUG status =======")

    result, output = ctrl.exec_run('systemctl is-active bluechi-agent')
    print("======== DOUG is-active =======")
    print(output)
    print(result)
    print("======== DOUG is-active =======")
    #assert result == 0
    #assert output == 'active'

    result, output = ctrl.exec_run('bluechictl status node-foo bluechi-agent.service')
    print("======== DOUG status node-foo =======")
    print(output)
    print(result)
    print("======== DOUG status node-foo =======")
    #assert result == 0

    # Split the output into lines
    lines = output.strip().split("\n")

    # Find the line that starts with "bluechi-agent.service"
    unit_line = next(line for line in lines if line.startswith("bluechi-agent.service"))

    # Split the unit line into columns using the "|" character
    columns = unit_line.split("|")

    # Extract the third item (ACTIVE) and remove surrounding whitespace
    active_value = columns[2].strip()
    assert active_value == 'active'


@pytest.mark.timeout(30)
def test_agent_foo_startup(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    bluechi_node_default_config.node_name = "node-foo"

    bluechi_ctrl_default_config.allowed_node_names = [CONTROLLER_NAME, NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(bluechi_node_default_config)

    bluechi_test.run(foo_startup_verify)
