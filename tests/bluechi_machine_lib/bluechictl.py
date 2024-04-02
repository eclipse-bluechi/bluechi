#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import copy
import signal
import subprocess
import tempfile
import threading
import time
from typing import List, Pattern, Set, Tuple

bluechictl_executable = ["/usr/bin/bluechictl"]


class FileFollower:
    def __init__(self, file_name):
        self.pos = 0
        self.file_name = file_name
        self.file_desc = None

    def __enter__(self):
        self.file_desc = open(self.file_name, mode="r")
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        if exception_type:
            print(
                f"Exception raised: excpetion_type='{exception_type}', "
                f"exception_value='{exception_value}', exception_traceback: {exception_traceback}"
            )
        if self.file_desc:
            self.file_desc.close()

    def __iter__(self):
        while self.new_lines():
            self.seek()
            line = self.file_desc.read().split("\n")[0]
            yield line

            self.pos += len(line) + 1

    def seek(self):
        self.file_desc.seek(self.pos)

    def new_lines(self):
        self.seek()
        return "\n" in self.file_desc.read()


class EventEvaluator:
    def process_line(self, line: str) -> None:
        pass

    def processing_finished(self) -> bool:
        pass


class SimpleEventEvaluator(EventEvaluator):
    def __init__(self, expected_patterns: Set[Pattern]):
        self.expected_patterns = copy.copy(expected_patterns)

    def process_line(self, line: str) -> None:
        for pattern in copy.copy(self.expected_patterns):
            if pattern.match(line):
                print(f"Matched line '{line}'")
                self.expected_patterns.remove(pattern)
                break

    def processing_finished(self) -> bool:
        return len(self.expected_patterns) == 0


class BackgroundRunner:
    def __init__(self, args: List, evaluator: EventEvaluator, timeout: int = 10):
        self.command = bluechictl_executable + args
        self.evaluator = evaluator
        self.thread = threading.Thread(target=self.process_events)
        self.failsafe_thread = threading.Thread(target=self.timeout_guard, daemon=True)
        self.bluechictl_proc = None
        self.timeout = timeout

    def process_events(self):
        print("Starting process_events")
        with tempfile.NamedTemporaryFile() as out_file:
            try:
                self.bluechictl_proc = subprocess.Popen(
                    self.command, stdout=out_file, bufsize=1
                )

                with FileFollower(out_file.name) as bluechictl_out:
                    finished = False
                    while self.bluechictl_proc.poll() is None and not finished:
                        for line in bluechictl_out:
                            print(f"Evaluating line '{line}'")
                            self.evaluator.process_line(line)
                            if self.evaluator.processing_finished():
                                print(
                                    "BluechiCtl output evaluation successfully finished"
                                )
                                finished = True
                                break

                        # Wait for the new output from bluechictl monitor
                        time.sleep(0.5)
            finally:
                print("Finalizing process")
                if self.bluechictl_proc:
                    self.bluechictl_proc.send_signal(signal.SIGINT)
                    self.bluechictl_proc.wait()
                    self.bluechictl_proc = None

    def timeout_guard(self):
        time.sleep(self.timeout)
        print(f"Waiting for expected output in {self.command} has timed out")
        self.bluechictl_proc.send_signal(signal.SIGINT)
        self.bluechictl_proc.wait()

    def start(self):
        print("Starting thread...")
        self.thread.start()
        self.failsafe_thread.start()

    def stop(self):
        self.thread.join()


class BluechiCtl:
    def run(self, args: List) -> Tuple[int, str, str]:
        command = bluechictl_executable + args
        process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        print(f"Executing of command '{process.args}' started")
        out, err = process.communicate()

        out = out.decode("utf-8")
        err = err.decode("utf-8")

        print(
            f"Executing of command '{process.args}' finished with result '{process.returncode}', "
            f"stdout '{out}', stderr '{err}'"
        )

        return process.returncode, out, err

    def metrics_listen(self) -> Tuple[int, str, str]:
        return self.run(["metrics", "listen"])

    def metrics_enable(self) -> Tuple[int, str, str]:
        return self.run(["metrics", "enable"])

    def metrics_disable(self) -> Tuple[int, str, str]:
        return self.run(["metrics", "disable"])

    def unit_start(self, node, unit) -> Tuple[int, str, str]:
        return self.run(["start", node, unit])

    def unit_stop(self, node, unit) -> Tuple[int, str, str]:
        return self.run(["stop", node, unit])

    def unit_restart(self, node, unit) -> Tuple[int, str, str]:
        return self.run(["restart", node, unit])

    def status_unit(self, node, unit) -> Tuple[int, str, str]:
        return self.run(["status", node, unit])

    def status_node(self, node) -> Tuple[int, str, str]:
        return self.run(["status", node])

    def status(self) -> Tuple[int, str, str]:
        return self.run(["status"])
