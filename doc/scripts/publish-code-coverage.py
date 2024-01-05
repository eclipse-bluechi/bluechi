# SPDX-License-Identifier: LGPL-2.1-or-later

import mkdocs_gen_files
import subprocess

with mkdocs_gen_files.open("code-coverage.md", "w") as f:
    print("# Code coverage report for BlueChi", file=f)
    print("", file=f)

    commit_hash = 'NA'
    with open('doc/code-coverage/commit-hash.txt') as ch_f:
        commit_hash = ch_f.readline()
    print(
        f"""Here is the current code coverage report created from commit
        [{commit_hash}](https://github.com/eclipse-bluechi/bluechi/commit/{commit_hash}):""",
        file=f)

    print("", file=f)
    print("```", file=f)

    cc_content = None
    process = subprocess.Popen(
        'lcov -l doc/code-coverage/merged.info -q',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = process.communicate()
    out = out.decode("utf-8")
    err = err.decode("utf-8")

    if process.returncode == 0:
        cc_content = out
    else:
        cc_content = f"Error '{process.returncode}' generating code coverage report: '{err}'"
    print(cc_content, file=f)

    print("```", file=f)
