# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import pathlib

from typing import Dict
from bluechi_test import command
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig
from bluechi_test.test import BluechiTest
from bluechi_test.util import read_file

LOGGER = logging.getLogger(__name__)


def merge_all_info_files(start_path, merge_file_name):
    lcov_list = list()
    lcov_list.append("lcov")

    root = pathlib.Path(start_path)

    for file_path in root.glob("**/*.info"):
        file = str(file_path)
        LOGGER.debug(f"Found info file '{file}'")
        lcov_list.append("-a")
        lcov_list.append(file)

    lcov_list.append("-o")
    lcov_list.append(merge_file_name)

    command.Command(args=" ".join(lcov_list)).run()


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    path_to_info_files = os.environ["TMT_TREE"].split('/tree')[0] + "/execute/data/"
    path_to_tests_results = os.environ["TMT_TREE"].split('/tree')[0] + "/report/default-0"
    merge_file_name = "merged.info"
    merge_file_full_path = path_to_tests_results + "/" + merge_file_name
    merge_dir = "/tmp"
    report_dir_name = "/report"

    merge_all_info_files(path_to_info_files, merge_file_full_path)

    if os.path.exists(merge_file_full_path):
        LOGGER.debug(f"Generating report for merged info file '{merge_file_full_path}'")
        content = read_file(merge_file_full_path)
        ctrl.create_file(merge_dir, merge_file_name, content)
        ctrl.exec_run(f"genhtml {merge_dir}/{merge_file_name} --output-directory={report_dir_name}")
        ctrl.get_file(report_dir_name, path_to_tests_results)


def test_create_coverag_report(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig):

    if (os.getenv("WITH_COVERAGE") == "1"):
        LOGGER.info("Code coverage report generation started")
        bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
        bluechi_test.run(exec)
        LOGGER.info("Code coverage report generation finished")
