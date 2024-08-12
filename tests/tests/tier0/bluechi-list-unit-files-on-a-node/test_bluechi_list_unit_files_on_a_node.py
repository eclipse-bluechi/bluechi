#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def parse_bluechictl_output(output: str) -> Dict[str, Dict[str, str]]:
    line_pat = re.compile(
        r"""\s*(?P<node_name>[\S]+)\s*\|
                              \s*((?:[^/]*/)*)(?P<unit_file_path>[\S]+)\s*\|
                              \s*(?P<enablement_status>[\S]+)\s*""",
        re.VERBOSE,
    )
    result = {}
    for line in output.splitlines():
        if line.startswith("NODE ") or line.startswith("===="):
            continue

        match = line_pat.match(line)
        if not match:
            raise Exception(
                f"Error parsing bluechictl list-unit-files output, invalid line: '{line}'"
            )

        node_unit_files = result.get(match.group("node_name"))
        if not node_unit_files:
            node_unit_files = {}
            result[match.group("node_name")] = node_unit_files

        if match.group("unit_file_path") in node_unit_files:
            raise Exception(
                f"Error parsing bluechictl list-unit-files output, unit file already reported, line: '{line}'"
            )

        node_unit_files[match.group("unit_file_path")] = match.group(
            "enablement_status"
        )

    return result


def verify_unit_files(all_unit_files: Dict[str, str], output: str, node_name: str):
    esc_seq = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")
    line_pat = re.compile(
        r"""\s*(?P<unit_file_path>\S+)
                                   \s+(?P<enablement_status>\S+)
                                   \s+.*$
                               """,
        re.VERBOSE,
    )
    for line in output.splitlines():
        line = esc_seq.sub("", line)

        match = line_pat.match(line)
        if not match:
            raise Exception(
                f"Error parsing systemctl list-unit-files output, invalid line: '{line}'"
            )

        found = all_unit_files.get(match.group("unit_file_path"))
        if not found or match.group("enablement_status") != found:
            raise Exception(
                "Unit file '{}' with enablement status '{}' reported by systemctl"
                " on node '{}', but not reported by bluechictl".format(
                    match.group("unit_file_path"),
                    match.group("enablement_status"),
                    node_name,
                )
            )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_foo_name]

    all_res, all_out = ctrl.bluechictl.list_unit_files(node_name=node_foo_name)
    assert all_res == 0
    all_unit_files = parse_bluechictl_output(all_out)

    foo_res, foo_out = node_foo.systemctl.list_unit_files()
    assert foo_res == 0
    verify_unit_files(all_unit_files[node_foo_name], foo_out, node_foo_name)


def test_bluechi_list_unit_files_on_a_node(
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
