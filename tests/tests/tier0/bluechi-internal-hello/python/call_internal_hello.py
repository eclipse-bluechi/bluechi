#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.connection import AddressedMessageBus

node = "node-foo"


class TestListenerAddedAsPeer(unittest.TestCase):
    def test_call_internal_hello(self):

        port = ""
        with open("/tmp/port", "r") as f:
            port = f.read().strip()
        assert port is not None

        # connect to controller locally
        peer_bus = AddressedMessageBus(f"tcp:host=127.0.0.1,port={port}")
        peer_proxy = peer_bus.get_proxy(
            service_name="org.eclipse.bluechi",
            object_path="/org/freedesktop/DBus",
            interface_name="org.freedesktop.DBus",
        )
        resp = peer_proxy.Hello()
        assert resp == ":1.0"


if __name__ == "__main__":
    unittest.main()
