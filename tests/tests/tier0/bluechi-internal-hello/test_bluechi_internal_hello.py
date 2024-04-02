#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):

    # pass port information to machine
    ctrl.create_file(os.path.join("/", "tmp"), "port", ctrl.config.port)

    result, output = ctrl.run_python(os.path.join("python", "call_internal_hello.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_internal_hello(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
