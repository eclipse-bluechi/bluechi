#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from typing import Dict

from bluechi_test.config import (
    BluechiAgentConfig,
    BluechiControllerConfig,
    BluechiControllerPerNodeConfig,
)
from bluechi_test.constants import NODE_CTRL_NAME
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

LOGGER = logging.getLogger(__name__)

NODE_BAR = "node-bar"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    node_bar = nodes[NODE_BAR]

    tgt_svc = SimpleRemainingService(name="target.service")
    tgt_svc.set_option(Section.Service, Option.ExecStart, "/bin/false")

    node_bar.install_systemd_service(tgt_svc)

    req_svc = SimpleRemainingService(name="requesting.service")
    req_svc.set_option(
        Section.Unit, Option.BindsTo, "bluechi-proxy@node-bar_target.service"
    )
    req_svc.set_option(
        Section.Unit, Option.After, "bluechi-proxy@node-bar_target.service"
    )
    req_svc.set_option(Section.Service, Option.ExecStart, "/bin/true")
    req_svc.set_option(Section.Service, Option.RemainAfterExit, "yes")

    ctrl.install_systemd_service(req_svc)

    result, output = ctrl.systemctl.start_unit(req_svc.name, check_result=False)

    # Depending how fast the failure of target.service gets propagated, the
    # job of the requesting service ends up being canceled.
    # To account for this race condition, we check both cases.
    if result != 0:
        assert f"Job for {req_svc.name} canceled" in str(output)
    assert ctrl.wait_for_unit_state_to_be(req_svc.name, "inactive")
    assert node_bar.wait_for_unit_state_to_be(tgt_svc.name, "failed")


def test_proxy_service_propagate_target_service_failure(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = NODE_BAR

    bluechi_test.add_bluechi_agent_config(node_bar_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_BAR]
    bluechi_ctrl_default_config.per_node_config.append(
        BluechiControllerPerNodeConfig(NODE_CTRL_NAME, proxy_to=[NODE_BAR]),
    )

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
