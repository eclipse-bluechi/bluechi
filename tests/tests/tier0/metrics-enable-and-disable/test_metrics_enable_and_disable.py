#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import SimpleRemainingService
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

node_foo_name = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    foo = nodes[node_foo_name]
    service = SimpleRemainingService()

    foo.install_systemd_service(service)
    assert foo.wait_for_unit_state_to_be(service.name, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "metrics_is_enabled.py"))
    if result == 0:
        raise Exception(f"Metrics not disabled: {output}")
    assert foo.wait_for_unit_state_to_be(service.name, "inactive")

    ctrl.bluechictl.enable_metrics()

    result, output = ctrl.run_python(os.path.join("python", "metrics_is_enabled.py"))
    if result != 0:
        raise Exception(f"Metrics not enabled: {output}")
    assert foo.wait_for_unit_state_to_be(service.name, "inactive")

    ctrl.bluechictl.disable_metrics()

    result, output = ctrl.run_python(os.path.join("python", "metrics_is_enabled.py"))
    if result == 0:
        raise Exception(f"Metrics not disabled: {output}")
    assert foo.wait_for_unit_state_to_be(service.name, "inactive")


def test_metrics_enable_and_disable(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
