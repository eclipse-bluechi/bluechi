# SPDX-License-Identifier: GPL-2.0-or-later

import re
import socket
from typing import Tuple


def read_file(local_file: str) -> (str | None):
    content = None
    with open(local_file, 'r') as fh:
        content = fh.read()
    return content


def get_primary_ip() -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        # connect to an arbitrary address
        s.connect(("254.254.254.254", 1))
        ip = s.getsockname()[0]
    except Exception as ex:
        print(f"Failed to get IP, falling back to localhost. Exception: {ex}")
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

def filter_hirtectl_list_units(output: str, unit: str) -> Tuple[str, str, str]:
    masked_unit = unit.replace('.', '\\.')
    matches = re.search(f"({masked_unit})(\s+\|)(\s+)(.+)(\|\s+)(.+)(\n)", output)
    if matches is not None and len(matches.groups()) == 7:
        groups = matches.groups()
        return (groups[0], groups[3], groups[5])
    return ("", "", "")
