#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import subprocess
import time
import unittest
from threading import Thread

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor

node_name_foo = "node-foo"

service_simple = "simple.service"


class TestMonitorSpecificNodeAndUnit(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()
        self.mgr = Controller()

        self.inactive = False
        self.active = True

    def run_command(self, args, shell=True, **kwargs):
        process = subprocess.Popen(
            args, shell=shell, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
        )
        print(f"Executing of command '{process.args}' started")
        out, err = process.communicate()

        out = out.decode("utf-8")
        err = err.decode("utf-8")

        print(
            f"Executing of command '{process.args}' finished with result '{process.returncode}',"
            " stdout '{out}', stderr '{err}'"
        )

        return process.returncode, out, err

    def process_events(self):
        self.loop.run()

    def timeout_guard(self):
        time.sleep(10)
        print(
            f"Loop timeout - service '{service_simple}' on node '{node_name_foo}' "
            f"was not successfully restarted on time"
        )
        self.loop.quit()

    def test_monitor_specific_node_and_unit(self):

        monitor_path = self.mgr.create_monitor()
        monitor = Monitor(monitor_path=monitor_path)

        def on_unit_state_changed(
            node: str, unit: str, active_state: str, sub_state: str, reason: str
        ) -> None:
            print(
                f"Received state change for node '{node}', unit '{unit}', "
                f"active_state '{active_state}', sub_state '{sub_state}'"
            )
            if active_state == "inactive" and self.active and not self.inactive:
                # Active -> Inactive
                print(f"Service '{unit}' on node '{node}' was stopped successfully")
                self.active = False
                self.inactive = True
            elif active_state == "active" and self.inactive:
                # Inactive -> Active
                print(f"Service '{unit}' on node '{node}' was started successfully")
                self.active = True
                self.loop.quit()
            else:
                print(f"Ignoring status change to '{active_state}'")

        monitor.on_unit_state_changed(on_unit_state_changed)

        monitor.subscribe(node_name_foo, service_simple)

        t = Thread(target=self.process_events)
        # mark the failsafe thread as daemon so it stops when the main process stops
        failsafe_thread = Thread(target=self.timeout_guard, daemon=True)
        t.start()
        failsafe_thread.start()

        res, _, _ = self.run_command(
            f"bluechictl restart {node_name_foo} {service_simple}"
        )
        assert res == 0

        t.join()

        assert self.inactive
        assert self.active


if __name__ == "__main__":
    unittest.main()
