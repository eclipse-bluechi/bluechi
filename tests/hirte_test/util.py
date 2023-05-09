# SPDX-License-Identifier: GPL-2.0-or-later

def read_file(local_file: str) -> (str | None):
    content = None
    with open(local_file, 'r') as fh:
        content = fh.read()
    return content
