# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig


def exec(ctrl: BlueChiControllerMachine, _: Dict[str, BlueChiAgentMachine]):

    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")

    original_cfg = ctrl.ctrl_config.deep_copy()

    # port contains an 'O' instead of a '0'
    new_cfg = original_cfg.deep_copy()
    new_cfg.port = "542O"
    ctrl.create_file(new_cfg.get_confd_dir(), new_cfg.file_name, new_cfg.serialize())
    ctrl.exec_run("systemctl restart bluechi-controller")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "failed")

    # port out of range
    new_cfg = original_cfg.deep_copy()
    new_cfg.port = "65538"
    ctrl.create_file(new_cfg.get_confd_dir(), new_cfg.file_name, new_cfg.serialize())
    # to ensure that a restart is possible, reset the lock due to too many failed attempts
    ctrl.exec_run("systemctl reset-failed bluechi-controller")
    ctrl.exec_run("systemctl restart bluechi-controller")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "failed")

    # valid config again
    new_cfg = original_cfg.deep_copy()
    ctrl.create_file(new_cfg.get_confd_dir(), new_cfg.file_name, new_cfg.serialize())
    # to ensure that a restart is possible, reset the lock due to too many failed attempts
    ctrl.exec_run("systemctl reset-failed bluechi-controller")
    ctrl.exec_run("systemctl restart bluechi-controller")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")


def test_agent_invalid_port(bluechi_test: BlueChiTest, bluechi_ctrl_default_config: BlueChiControllerConfig):

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.run(exec)
