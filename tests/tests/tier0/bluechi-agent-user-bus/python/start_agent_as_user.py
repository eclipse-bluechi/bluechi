#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi_machine_lib.util import run_command

service = "org.eclipse.bluechi.Agent"
object = "/org/eclipse/bluechi"
interface = "org.eclipse.bluechi.Agent"


class TestAgentStartAsUser(unittest.TestCase):

    def test_agent_start_as_user(self):
        result, _, _ = run_command("systemctl --user start bluechi-agent")
        assert result == 0

        while True:
            result, output, _ = run_command(
                f"busctl --user get-property {service} {object} {interface} Status"
            )
            if output == 's "offline"':
                break
            time.sleep(0.5)

        result, _, _ = run_command("systemctl --user stop bluechi-agent")
        assert result == 0


if __name__ == "__main__":
    unittest.main()
