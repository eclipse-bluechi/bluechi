#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import signal
import subprocess
import threading
import time
import unittest

from bluechi_machine_lib.util import run_command

service = "org.eclipse.bluechi.Agent"
object = "/org/eclipse/bluechi"
interface = "org.eclipse.bluechi.Agent"


class TestAgentStartAsUser(unittest.TestCase):

    def run_agent(self):
        command = "/usr/libexec/bluechi-agent -u"
        self.process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True
        )
        print(f"Executing of command '{command}' started")
        self.process.communicate()
        print(f"Executing of command '{command}' stopped")
        self.finished = True

    def timeout_guard(self):
        time.sleep(10)
        print("Waiting for agent to start has timed out")
        self.process.send_signal(signal.SIGINT)
        self.process.wait()

    def test_agent_start_as_user(self):
        success = False
        self.finished = False
        thread = threading.Thread(target=self.run_agent, daemon=True)
        failsafe_thread = threading.Thread(target=self.timeout_guard, daemon=True)
        thread.start()
        failsafe_thread.start()

        while not self.finished:
            _, output, _ = run_command(
                f"busctl --user get-property {service} {object} {interface} Status"
            )
            if output == 's "offline"':
                print("bluechi-agent connected to user bus")
                success = True
                self.finished = True
            else:
                time.sleep(0.5)

        print("Sending SIGINT to agent process")
        self.process.send_signal(signal.SIGINT)
        thread.join()
        assert success


if __name__ == "__main__":
    unittest.main()
