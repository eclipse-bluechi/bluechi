#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import threading
import time
import unittest
from queue import Empty, Queue
from subprocess import PIPE, Popen
from typing import List

from bluechi.api import Node

node = "node-foo"
service = "simple.service"


class TestListenerNotAddedAsPeer(unittest.TestCase):
    def test_listener_not_added_as_peer(self):

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
        process_monitor_listener.stdout.readline().strip()

        # start the unit on the node monitored and wait a bit for all signals to be processed
        node_foo = Node(node)
        node_foo.start_unit(service, "replace")
        time.sleep(1)

        received_events = self.read_no_wait(process_monitor_listener.stdout)
        # the listener is not added as peer to the monitor,
        # so it shouldn't receive any signal and Empty exception is raised
        assert len(received_events) == 0

        process_monitor_owner.terminate()
        process_monitor_listener.terminate()

    ##################
    # Helper functions

    worker_queue: Queue = None
    worker: threading.Thread = None

    def read_no_wait(self, out) -> List[str]:
        """
        Helper function.
        stdout.readline() is a blocking call, use threads to make it non-blocking.
        """

        def enqueue_output(out, queue):
            while True:
                event = out.readline().strip()
                if event == "":
                    break
                queue.put(event)

        if self.worker_queue is None:
            self.worker_queue = Queue()
        if self.worker is None:
            self.worker = threading.Thread(
                target=enqueue_output, args=(out, self.worker_queue)
            )
            self.worker.daemon = True
            self.worker.start()

            # wait a bit for the new worker to catch up
            time.sleep(0.5)

        res = []
        while True:
            try:
                res.append(self.worker_queue.get_nowait())
            except Empty:
                break

        return res


if __name__ == "__main__":
    unittest.main()
