#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import threading
import time
from typing import Dict

from bluechi_test.bluechictl import BluechiCtl
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.service import Option, Section, SimpleRemainingService
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


class ResultFuture:
    def __init__(self) -> None:
        self.result = ""


def start_unit(bluechictl: BluechiCtl, future: ResultFuture):
    _, output = bluechictl.start_unit(
        node_foo_name, SimpleRemainingService().name, check_result=False
    )
    future.result = output


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    foo = nodes[node_foo_name]

    simple_service = SimpleRemainingService()
    simple_service.set_option(Section.Service, Option.ExecStartPre, "/bin/sleep 20")

    foo.install_systemd_service(simple_service)
    assert foo.wait_for_unit_state_to_be(simple_service.name, "inactive")

    # bluechictl start waits for the unit start-up to complete (successfully or failed)
    # save the result for later evaluation (should be cancelled)
    result_future = ResultFuture()
    start_worker = threading.Thread(
        target=start_unit,
        args=(
            ctrl.bluechictl,
            result_future,
        ),
    )
    start_worker.start()

    # wait a bit for the start flow to proceed
    time.sleep(0.5)
    ctrl.systemctl.stop_unit("bluechi-controller")
    start_worker.join()

    assert "cancelled due to shutdown" in result_future.result
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "inactive")
    # and started systemd unit on foo should still be in activating state (not affected by shutdown)
    assert foo.systemctl.get_unit_state(simple_service.name) == "activating"


def test_bluechi_request_cancel(
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
