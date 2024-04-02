#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
from subprocess import PIPE, Popen

from dasbus.error import DBusError

from bluechi.api import Monitor

node = "node-foo"
service = "simple.service"


class TestListenerAddedMoreThanOnce(unittest.TestCase):
    def test_listener_added_more_than_once(self):

        process_monitor_owner = Popen(
            ["python3", "/var/create_monitor.py", node, service],
            stdout=PIPE,
            universal_newlines=True,
        )
        monitor_path = process_monitor_owner.stdout.readline().strip()

        process_monitor_listener = Popen(
            ["python3", "/var/listen.py", monitor_path],
            stdout=PIPE,
            universal_newlines=True,
        )
        bus_id = process_monitor_listener.stdout.readline().strip()

        monitor = Monitor(monitor_path)
        monitor.add_peer(bus_id)
        with self.assertRaises(DBusError):
            monitor.add_peer(bus_id)

        process_monitor_owner.terminate()
        process_monitor_listener.terminate()


if __name__ == "__main__":
    unittest.main()
