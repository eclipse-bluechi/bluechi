# SPDX-License-Identifier: GPL-2.0-or-later

import random
import string
import socket

from typing import Union


def read_file(local_file: str) -> Union[str, None]:
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


def assemble_bluechi_dep_service_name(unit_name: str) -> str:
    return f"bluechi-dep@{unit_name}"


def assemble_bluechi_proxy_service_name(node_name: str, unit_name: str) -> str:
    return f"bluechi-proxy@{node_name}_{unit_name}"


def get_random_name(name_length: int) -> str:
    # choose from all lowercase letter
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(name_length))
