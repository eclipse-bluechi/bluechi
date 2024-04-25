#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import signal
import subprocess
from typing import Tuple


def run_command(command: str) -> Tuple[int, str, str]:
    process = subprocess.Popen(
        command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True
    )
    print(f"Executing of command '{process.args}' started")
    out, err = process.communicate()

    out = out.decode("utf-8").strip()
    err = err.decode("utf-8").strip()

    print(
        f"Executing of command '{process.args}' finished with result '{process.returncode}', "
        f"stdout '{out}', stderr '{err}'"
    )

    return process.returncode, out.strip(), err


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
