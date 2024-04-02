#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import pathlib
from typing import Dict

from bluechi_test.config import BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    path_to_info_files = os.environ["TMT_TREE"].split("/tree")[0] + "/execute/data/"
    path_to_tests_results = (
        os.environ["TMT_TREE"].split("/tree")[0] + "/report/default-0"
    )
    merge_file_name = "merged.info"
    merge_dir = "/tmp"
    report_dir_name = "/report"

    LOGGER.debug("Setting up source code directory for generating coverage report")
    ctrl.exec_run("/usr/share/bluechi-coverage/bin/setup-src-dir-for-coverage.sh")

    # Copy info files to the ctrl container and run lcov command
    LOGGER.debug(
        f"Merging info files from integration tests to '{merge_dir}/{merge_file_name}'"
    )
    root = pathlib.Path(path_to_info_files)
    lcov_list = list()
    lcov_list.append("lcov")
    for file_path in root.glob("**/*.info"):
        LOGGER.debug(f"Found info file '{str(file_path)}'")
        content = read_file(file_path)
        ctrl.create_file(merge_dir, file_path.name, content)
        lcov_list.append("-a")
        lcov_list.append(f"{merge_dir}/{file_path.name}")

    # Append unit tests code coverage results
    lcov_list.append("-a")
    lcov_list.append("/var/tmp/bluechi-coverage/unit-test-results/coverage.info")

    # Skip creating coverage report when no .info files are found
    if len(lcov_list) == 3:
        LOGGER.info(f"No .info files found in {root.name}, skipping...")
        return

    lcov_list.append("-o")
    lcov_list.append(f"{merge_dir}/{merge_file_name}")
    result, output = ctrl.exec_run(" ".join(lcov_list))
    if result != 0:
        raise Exception(
            f"Error merging info files from each integration test: {output}"
        )

    LOGGER.debug(
        f"Generating report for merged info file '{merge_dir}/{merge_file_name}'"
    )
    result, output = ctrl.exec_run(
        f"genhtml {merge_dir}/{merge_file_name} --output-directory={report_dir_name}"
    )
    if result != 0:
        raise Exception(f"Failed to do run genthtml inside the container: {output}")
    ctrl.get_file(report_dir_name, path_to_tests_results)
    ctrl.get_file(f"{merge_dir}/{merge_file_name}", path_to_tests_results)


def test_create_coverag_report(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    if os.getenv("WITH_COVERAGE") == "1":
        LOGGER.info("Code coverage report generation started")
        bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
        bluechi_test.run(exec)
        LOGGER.info("Code coverage report generation finished")
