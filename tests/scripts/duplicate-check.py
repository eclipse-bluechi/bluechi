#!/usr/bin/env python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import sys

import fmf


# TODO:
#   Remove when
#   https://github.com/teemtee/tmt/issues/2939
#   has been resolved and feature is in new release
def has_duplicate_id(fmf_tree: fmf.Tree):
    duplicate_ids = dict()
    for leave in fmf_tree.climb():
        id = leave.data.get("id")
        if id is None:
            continue
        if id not in duplicate_ids:
            duplicate_ids[id] = []
        duplicate_ids[id].append(leave.name)

    exit_code = 0
    for id, tests in duplicate_ids.items():
        if len(tests) > 1:
            print(f"Duplicate ID '{id}' detected for tests:")
            for test in tests:
                print(f"\t{test}")
            exit_code = 1
    return exit_code


def has_duplicate_summary(fmf_tree: fmf.Tree):
    duplicate_summaries = dict()
    for leave in fmf_tree.climb():
        summary = leave.data.get("summary")
        if summary is None:
            continue
        if summary not in duplicate_summaries:
            duplicate_summaries[summary] = []
        duplicate_summaries[summary].append(leave.name)

    exit_code = 0
    for summary, tests in duplicate_summaries.items():
        if len(tests) > 1:
            print("Duplicate summary detected for tests:")
            for test in tests:
                print(f"\t{test}")
            exit_code = 1
    return exit_code


if __name__ == "__main__":
    fmf_tree = fmf.Tree(os.getcwd())
    res_id = has_duplicate_id(fmf_tree)
    res_sum = has_duplicate_summary(fmf_tree)
    sys.exit(res_id + res_sum)
