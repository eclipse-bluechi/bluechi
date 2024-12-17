#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import inspect
import logging
import os
import random
import re
import signal
import socket
import string
from typing import Union

LOGGER = logging.getLogger(__name__)


def read_file(local_file: str) -> Union[str, None]:
    content = None
    with open(local_file, "r") as fh:
        content = fh.read()
    return content


def get_primary_ip(address: str = "254.254.254.254") -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        # connect to an arbitrary address
        s.connect((address, 1))
        ip = s.getsockname()[0]
    except Exception:
        LOGGER.exception("Failed to get IP, falling back to localhost.")
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
    return "".join(random.choice(letters) for _ in range(name_length))


_ANSI_SEQUENCE = re.compile(
    r"""
    \x1B  # ESC
    (?:   # 7-bit C1 Fe (except CSI)
        [@-Z\\-_]
    |     # or [ for CSI, followed by a control sequence
        \[
        [0-?]*  # Parameter bytes
        [ -/]*  # Intermediate bytes
        [@-~]   # Final byte
    )
""",
    re.VERBOSE,
)


def remove_control_chars(value: str) -> str:
    return _ANSI_SEQUENCE.sub("", value)


class Timeout:
    def __init__(self, seconds=1, error_message="Timeout"):
        self.seconds = seconds
        self.error_message = error_message

    def handle_timeout(self, signum, frame):
        raise TimeoutError(self.error_message)

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


def get_env_value(env_var: str, default_value: str) -> str:
    value = os.getenv(env_var)
    if value is None:
        return default_value
    return value


def safely_parse_int(input: str, default: int) -> int:
    if input.isdigit():
        return int(input)
    return default


def _get_test_env_value(varname: str, test_file: str, default_value: str) -> str:
    test_name = os.path.basename(os.path.dirname(test_file))
    envvar = f"TEST_{test_name.upper().replace('-', '_')}_{varname.upper()}"
    return get_env_value(envvar, default_value)


def get_test_env_value(varname: str, default_value: str) -> str:
    test_file = inspect.stack()[1].filename
    return _get_test_env_value(varname, test_file, default_value)


def get_test_env_value_int(varname: str, default_value: int) -> int:
    test_file = inspect.stack()[1].filename
    value = _get_test_env_value(varname, test_file, "")
    return safely_parse_int(value, default_value)
