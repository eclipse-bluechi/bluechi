#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import subprocess
from typing import Union

LOGGER = logging.getLogger(__name__)


class ExecutionError(Exception):
    """Raised when executed command has non-zero result code.The class contains the command result code and its stdout
    and strerr content.
    """

    def __init__(self, code: int, out: str, err: str) -> None:
        self.code = code
        self.out = out
        self.err = err

    def __str__(self):
        return f"Command failed with rc={self.code}. Stdout:\n{self.out}\nStderr:\n{self.err}\n"


class Command:
    """Execute a shell command and capture its output

    Arguments:
      args: A string, or a sequence of program arguments
      shell: If true, then command is executed within shell
    """

    def __init__(self, args: Union[str, list[str]], shell: bool = True) -> None:
        self.args = args
        self.shell = shell

    def run(self, **kwargs):
        """Run the command and return its stdout."""
        process = subprocess.Popen(
            self.args,
            shell=self.shell,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            **kwargs,
        )
        LOGGER.debug(f"Executing of command '{process.args}' started")
        out, err = process.communicate()

        out = out.decode("utf-8")
        err = err.decode("utf-8")

        LOGGER.debug(
            f"Executing of command '{process.args}' finished with result '{process.returncode}',"
            " stdout '{out}', stderr '{err}'"
        )

        if process.returncode != 0:
            raise ExecutionError(process.returncode, out, err)

        return out
