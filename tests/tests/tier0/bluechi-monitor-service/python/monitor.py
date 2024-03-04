# SPDX-License-Identifier: LGPL-2.1-or-later

import re
import signal
import subprocess
import tempfile
import threading
import time
import unittest

from bluechi.api import Node

node_name_foo = "node-foo"

service_simple = "simple.service"


class FileFollower:
    def __init__(self, file_name):
        self.pos = 0
        self.file_name = file_name
        self.file_desc = None

    def __enter__(self):
        self.file_desc = open(self.file_name, mode='r')
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        print(f"Exception raised: excpetion_type='{exception_type}', "
              f"exception_value='{exception_value}', exception_traceback: {exception_traceback}")
        if self.file_desc:
            self.file_desc.close()

    def __iter__(self):
        while self.new_lines():
            self.seek()
            line = self.file_desc.read().split('\n')[0]
            yield line

            self.pos += len(line) + 1

    def seek(self):
        self.file_desc.seek(self.pos)

    def new_lines(self):
        self.seek()
        return '\n' in self.file_desc.read()


class TestMonitorSpecificNodeAndUnit(unittest.TestCase):

    def setUp(self) -> None:
        self.bluechictl_proc = None
        self.state_pat = re.compile(r"\s*ActiveState: (?P<active_state>[\S]+)\s*")

        self.inactive = False
        self.active = True

    def timeout_guard(self):
        time.sleep(10)
        print(f"Loop timeout - service '{service_simple}' on node '{node_name_foo}' "
              f"was not successfully restarted on time")
        self.bluechictl_proc.send_signal(signal.SIGINT)
        self.bluechictl_proc.wait()

    def process_events(self):
        out_file = None

        with tempfile.NamedTemporaryFile() as out_file:
            try:
                self.bluechictl_proc = subprocess.Popen(
                    ["/usr/bin/bluechictl", "monitor", f"{node_name_foo}", f"{service_simple}"],
                    stdout=out_file,
                    bufsize=1)

                with FileFollower(out_file.name) as bluechictl_out:
                    events_received = False
                    while self.bluechictl_proc.poll() is None and not events_received:
                        for line in bluechictl_out:
                            print(f"Evaluating line '{line}'")
                            match = self.state_pat.match(line)
                            if not match:
                                print(f"Ignoring line '{line}'")
                                continue
                            active_state = match.group("active_state")
                            if active_state == "inactive" and self.active and not self.inactive:
                                # Active -> Inactive
                                print(f"Service '{service_simple}' on node '{node_name_foo}' was stopped successfully")
                                self.active = False
                                self.inactive = True
                            elif active_state == "active" and self.inactive:
                                # Inactive -> Active
                                print(f"Service '{service_simple}' on node '{node_name_foo}' was started successfully")
                                self.active = True
                                events_received = True
                                break

                        # Wait for the new output from bluechictl monitor
                        time.sleep(0.5)
            finally:
                if self.bluechictl_proc:
                    self.bluechictl_proc.send_signal(signal.SIGINT)
                    self.bluechictl_proc.wait()
                    self.bluechictl_proc = None

    def test_monitor_specific_node_and_unit(self):
        t = threading.Thread(target=self.process_events)
        # mark the failsafe thread as daemon so it stops when the main process stops
        failsafe_thread = threading.Thread(target=self.timeout_guard, daemon=True)
        t.start()
        failsafe_thread.start()

        node_foo = Node(node_name_foo)
        assert node_foo.restart_unit(service_simple, "replace") != ""

        t.join()

        assert self.inactive
        assert self.active


if __name__ == "__main__":
    unittest.main()
