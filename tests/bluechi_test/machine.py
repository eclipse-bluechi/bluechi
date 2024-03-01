# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import time
import traceback

from typing import Any, Iterator, Optional, Tuple, Union, List

from bluechi_test.client import Client
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.systemctl import SystemCtl
from bluechi_test.bluechictl import BluechiCtl
from bluechi_test.util import read_file, get_random_name

LOGGER = logging.getLogger(__name__)


class BluechiMachine():

    valgrind_log_directory = '/var/log/valgrind'
    valgrind_log_path_controller = '/var/log/valgrind/bluechi-controller-valgrind.log'
    valgrind_log_path_agent = '/var/log/valgrind/bluechi-agent-valgrind.log'

    gcda_file_location = '/var/tmp/bluechi-coverage'

    backup_file_suffix = '.backup'

    def __init__(self, name: str, client: Client) -> None:
        self.name = name
        self.client = client

        self.systemctl = SystemCtl(client)

        self.created_files: List[str] = []
        self.changed_files: List[str] = []

    def create_file(self, target_dir: str, file_name: str, content: str) -> None:
        target_file = os.path.join(target_dir, file_name)
        try:
            self.client.create_file(target_dir, file_name, content)
        except Exception as ex:
            LOGGER.error(f"Failed to create file '{target_file}': {ex}")
            traceback.print_exc()
            return

        # keep track of create file for later cleanup
        self.created_files.append(os.path.join(target_dir, file_name))

    def _track_changed_file(self, target_dir: str, file_name: str) -> None:
        target_file = os.path.join(target_dir, file_name)
        try:
            # create backup of original file only the first time
            if target_file in self.changed_files:
                return

            LOGGER.debug(f"Creating backup of file '{target_file}'...")
            backup_file = target_file + BluechiMachine.backup_file_suffix
            result, output = self.client.exec_run(f"cp {target_file} {backup_file}")
            if result != 0:
                raise Exception(output)
            self.changed_files.append(target_file)
        except Exception as ex:
            LOGGER.error(f"Failed to create backup of file '{target_file}': {ex}")
            traceback.print_exc()
            return

    def get_file(self, machine_path: str, local_path: str) -> None:
        self.client.get_file(machine_path, local_path)

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False, tty: bool = True) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.client.exec_run(command, raw_output, tty)

    def wait_for_unit_state_to_be(
            self,
            unit_name: str,
            expected_state: str,
            timeout: float = 5.0,
            delay: float = 0.5) -> bool:

        if self.systemctl.is_unit_in_state(unit_name, expected_state):
            return True

        start = time.time()
        while (time.time() - start) < timeout:
            time.sleep(delay)
            if self.systemctl.is_unit_in_state(unit_name, expected_state):
                return True

        LOGGER.error(f"Timeout while waiting for '{unit_name}' to reach state '{expected_state}'.")
        return False

    def systemctl_start_and_wait(self, service_name: str, sleep_after: float = 0.0) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]], Optional[bool]]:

        result, output = self.systemctl.start_unit(service_name)
        wait_result = self.wait_for_unit_state_to_be(service_name, "active")
        time.sleep(sleep_after)

        return result, output, wait_result

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        source_path = os.path.join(source_dir, service_file_name)
        content = read_file(source_path)

        LOGGER.debug(f"Copy local systemd service '{source_path}' to container path '{target_dir}'\
             with content:\n{content}")
        self.create_file(target_dir, service_file_name, content)
        self.systemctl.daemon_reload()

        # keep track of created service file to potentially stop it in later cleanup
        self.systemctl.tracked_services.append(service_file_name)

    def copy_container_script(self, script_file_name: str):
        curr_dir = os.getcwd()
        source_path = os.path.join(curr_dir, "..", "..", "..", "bluechi_test", "container_scripts", script_file_name)
        content = read_file(source_path)

        target_dir = os.path.join("/", "var")

        LOGGER.info(f"Copy container script '{source_path}' to container path '{curr_dir}'\
             with content:\n{content}")
        self.create_file(target_dir, script_file_name, content)

    def restart_with_config_file(self, config_file_location, service):
        unit_dir = "/usr/lib/systemd/system"
        service_file = f"{service}.service"

        self._track_changed_file(unit_dir, service_file)
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/libexec/{service} -c "
                             f"{config_file_location}' "
                             f"{os.path.join(unit_dir, service_file)}")
        self.systemctl.daemon_reload()
        self.systemctl.restart_unit(service_file)

    def wait_for_bluechi_agent(self):
        should_wait = True
        while should_wait:
            should_wait = not self.systemctl.service_is_active("bluechi-agent")

    def wait_for_bluechi_controller(self):
        should_wait = True
        while should_wait:
            should_wait = not self.systemctl.service_is_active("bluechi-controller")

    def enable_valgrind(self) -> None:
        unit_dir = "/usr/lib/systemd/system"
        controller_service = "bluechi-controller.service"
        agent_service = "bluechi-agent.service"

        self._track_changed_file(unit_dir, controller_service)
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                             f"--log-file={BluechiMachine.valgrind_log_path_controller} "
                             f"/usr/libexec/bluechi-controller' {os.path.join(unit_dir, controller_service)}")

        self._track_changed_file(unit_dir, agent_service)
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                             f"--log-file={BluechiMachine.valgrind_log_path_agent} /usr/libexec/bluechi-agent' "
                             f"{os.path.join(unit_dir, agent_service)}")

        self.client.exec_run(f"mkdir -p {BluechiMachine.valgrind_log_directory}")
        self.systemctl.daemon_reload()

    def run_python(self, python_script_path: str) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        target_file_dir = os.path.join("/", "tmp")
        target_file_name = get_random_name(10)
        content = read_file(python_script_path)

        # directly call create_file on client to bypass cleanup as
        # the script file will be removed after running it
        self.client.create_file(target_file_dir, target_file_name, content)

        target_file_path = os.path.join(target_file_dir, target_file_name)
        LOGGER.debug(f"Executing python script '{target_file_path}'")
        result, output = self.client.exec_run(f'python3 {target_file_path}')
        LOGGER.debug(f"Execute python script '{target_file_path}' finished with result '{result}' \
            and output:\n{output}")
        try:
            os.remove(target_file_path)
        finally:
            return result, output

    def gather_valgrind_logs(self, data_dir: str) -> None:
        try:
            self.client.get_file(BluechiMachine.valgrind_log_path_controller, data_dir)
        except Exception as ex:
            LOGGER.debug(f"Failed to get valgrind logs for controller: {ex}")

        try:
            self.client.get_file(BluechiMachine.valgrind_log_path_agent, data_dir)
        except Exception as ex:
            LOGGER.debug(f"Failed to get valgrind logs for agent: {ex}")

    def gather_journal_logs(self, data_dir: str) -> None:
        log_file = f"/tmp/journal-{self.name}.log"

        self.client.exec_run(
            f'bash -c "journalctl --no-pager > {log_file}"', tty=True)

        # track created logfile for later cleanup
        self.created_files.append(log_file)

        self.client.get_file(log_file, data_dir)

    def gather_coverage(self, data_coverage_dir: str) -> None:
        coverage_file = f"{BluechiMachine.gcda_file_location}/coverage-{self.name}.info"

        LOGGER.info(f"Generating info file '{coverage_file}' started")
        result, output = self.client.exec_run(
            f"/usr/share/bluechi-coverage/bin/gather-code-coverage.sh {coverage_file}")
        if result != 0:
            LOGGER.error(f"Failed to gather code coverage: {output}")
        LOGGER.info(f"Generating info file '{coverage_file}' finished")

        self.client.get_file(f"{coverage_file}", data_coverage_dir)

    def cleanup_valgrind_logs(self):
        self.client.exec_run(f"rm -f {BluechiMachine.valgrind_log_path_controller}")
        self.client.exec_run(f"rm -f {BluechiMachine.valgrind_log_path_agent}")

    def cleanup_journal_logs(self):
        self.client.exec_run("journalctl --flush --rotate")
        self.client.exec_run("journalctl --vacuum-time=1s")

    def cleanup_coverage(self):
        self.client.exec_run(f"rm -rf {BluechiMachine.gcda_file_location}/*")


class BluechiAgentMachine(BluechiMachine):

    def __init__(self, name: str, client: Client, agent_config: BluechiAgentConfig) -> None:
        super().__init__(name, client)

        self.config = agent_config

        # add confd file to container
        self.create_file(self.config.get_confd_dir(), self.config.file_name, self.config.serialize())


class BluechiControllerMachine(BluechiMachine):

    def __init__(self, name: str, client: Client, ctrl_config: BluechiControllerConfig) -> None:
        super().__init__(name, client)

        self.bluechictl = BluechiCtl(client)

        self.config = ctrl_config

        # add confd file to container
        self.create_file(self.config.get_confd_dir(), self.config.file_name, self.config.serialize())
