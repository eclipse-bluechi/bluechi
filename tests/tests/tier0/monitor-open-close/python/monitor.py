# SPDX-License-Identifier: GPL-2.0-or-later

import unittest

from dasbus.error import DBusError

from bluechi.api import Manager, Monitor

# Subscribing to a closed monitor should result in a DBusError since the monitor has removed.
# In this test we first subscribe to an open monitor to verify everything works, then close
# the monitor and expect an DBusError when subscribing on a closed monitor.


class TestOpenClose(unittest.TestCase):

    def test_open_close(self):
        mgr = Manager()
        monitor_path = mgr.create_monitor()
        monitor = Monitor(monitor_path=monitor_path)
        monitor.subscribe('not-important-node', 'not-important-unit')
        monitor.close()
        with self.assertRaises(DBusError):
            monitor.subscribe('not-important-node-2', 'not-important-unit-2')


if __name__ == "__main__":
    unittest.main()
