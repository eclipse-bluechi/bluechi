# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict
from bluechi_test.util import read_file
import subprocess

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


def merge_all_info_files(start_path):
    add_merged_file = ""
    all_bluchi_coverage_files = subprocess.check_output(
        f"find {start_path} -name bluechi-coverage", shell=True).decode()
    all_bluchi_coverage_files = all_bluchi_coverage_files.split("\n")
    all_bluchi_coverage_files.pop()
    for bluechi_coverage_dir in all_bluchi_coverage_files:
        for path in os.listdir(bluechi_coverage_dir):
            if "info" in path:
                info_file = os.path.join(bluechi_coverage_dir, path)
                if os.path.isfile(info_file):
                    if os.stat(info_file).st_size != 0:
                        os.system(
                            f'lcov --add-tracefile {info_file} {add_merged_file} -o merged.info > /dev/null')
                        add_merged_file = "-a merged.info"


def exec(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    path_to_info_files = os.environ["TMT_TREE"].split('/tree')[0] + "/execute/data/"
    path_to_tests_results = os.environ["TMT_TREE"].split('/tree')[0] + "/report/default-0"
    merge_file_name = "merged.info"
    merge_dir = "/tmp"
    report_dir_name = "/report"

    merge_all_info_files(path_to_info_files)
    if os.path.exists(merge_file_name):
        content = read_file(merge_file_name)
        ctrl.create_file(merge_dir, merge_file_name, content)
        ctrl.exec_run(f"genhtml {merge_dir}/{merge_file_name} --output-directory={report_dir_name}")
        ctrl.get_file(report_dir_name, path_to_tests_results)


def test_create_coverag_report(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    if (os.getenv("WITH_COVERAGE") == "1"):
        bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
        bluechi_test.run(exec)
