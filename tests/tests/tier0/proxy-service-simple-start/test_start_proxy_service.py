# SPDX-License-Identifier: GPL-2.0-or-later

import re
import pytest
from typing import Dict

from hirte_test.config import HirteControllerConfig, HirteNodeConfig
from hirte_test.container import HirteContainer
from hirte_test.test import HirteTest
from hirte_test.util import read_file


node_foo_name = "node-foo"
node_bar_name = "node-bar"


def exec(ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
    foo = nodes[node_foo_name]
    bar = nodes[node_bar_name]

    foo.create_file("/etc/systemd/system", "requesting.service", read_file("./systemd/requesting.service"))
    foo.systemctl_daemon_reload()
    bar.create_file("/etc/systemd/system", "simple.service", read_file("./systemd/simple.service"))
    bar.systemctl_daemon_reload()

    foo.wait_for_hirte_agent()
    bar.wait_for_hirte_agent()
    ctrl.wait_for_hirte()

    result, _ = ctrl.exec_run(f"hirtectl start {node_foo_name} requesting.service")
    assert result == 0

    # verify the units have been loaded
    result, output = ctrl.exec_run(f"hirtectl list-units {node_bar_name} | grep simple.service")
    assert result == 0
    assert re.search("active", output) is not None

    result, output = ctrl.exec_run(f"hirtectl list-units {node_foo_name} | grep simple.service")
    assert result == 0
    assert re.search("active", output) is not None


@pytest.mark.timeout(10)
def test_start_proxy_service(
        hirte_test: HirteTest,
        hirte_ctrl_default_config: HirteControllerConfig,
        hirte_node_default_config: HirteNodeConfig):

    node_foo_cfg = hirte_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    node_bar_cfg = hirte_node_default_config.deep_copy()
    node_bar_cfg.node_name = node_bar_name

    hirte_ctrl_default_config.allowed_node_names = [node_foo_name, node_bar_name]

    hirte_test.set_hirte_controller_config(hirte_ctrl_default_config)
    hirte_test.add_hirte_node_config(node_foo_cfg)
    hirte_test.add_hirte_node_config(node_bar_cfg)

    hirte_test.run(exec)
