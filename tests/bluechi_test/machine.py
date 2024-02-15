# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
import time
import traceback

from typing import Any, Iterator, Optional, Tuple, Union

from bluechi_test.client import Client, ContainerClient, SSHClient
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.systemctl import SystemCtl
from bluechi_test.bluechictl import BluechiCtl
from bluechi_test.util import read_file, get_random_name

LOGGER = logging.getLogger(__name__)


class BluechiMachine():

    valgrind_log_path_controller = '/var/log/valgrind/bluechi-controller-valgrind.log'
    valgrind_log_path_agent = '/var/log/valgrind/bluechi-agent-valgrind.log'

    def __init__(self, name: str, client: Client) -> None:
        self.name = name
        self.client = client

        self.systemctl = SystemCtl(client)

    def create_file(self, target_dir: str, file_name: str, content: str) -> None:
        target_file = os.path.join(target_dir, file_name)
        try:
            self.client.create_file(target_dir, file_name, content)
        except Exception as ex:
            LOGGER.error(f"Failed to create file '{target_file}': {ex}")
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
        self.client.create_file(target_dir, service_file_name, content)
        self.systemctl.daemon_reload()

    def copy_container_script(self, script_file_name: str):
        curr_dir = os.getcwd()
        source_path = os.path.join(curr_dir, "..", "..", "..", "bluechi_test", "container_scripts", script_file_name)
        content = read_file(source_path)

        target_dir = os.path.join("/", "var")

        LOGGER.info(f"Copy container script '{source_path}' to container path '{curr_dir}'\
             with content:\n{content}")
        self.client.create_file(target_dir, script_file_name, content)

    def restart_with_config_file(self, config_file_location, service):
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/libexec/{service} -c "
                             f"{config_file_location}' "
                             f"/usr/lib/systemd/system/{service}.service")
        self.systemctl.daemon_reload()
        self.systemctl.restart_unit(f"{service}.service")

    def wait_for_bluechi_agent(self):
        should_wait = True
        while should_wait:
            should_wait = not self.systemctl.service_is_active("bluechi-agent")

    def wait_for_bluechi_controller(self):
        should_wait = True
        while should_wait:
            should_wait = not self.systemctl.service_is_active("bluechi-controller")

    def enable_valgrind(self) -> None:
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                             f"--log-file={BluechiMachine.valgrind_log_path_controller} "
                             f"/usr/libexec/bluechi-controller' /usr/lib/systemd/system/bluechi-controller.service")
        self.client.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                             f"--log-file={BluechiMachine.valgrind_log_path_agent} /usr/libexec/bluechi-agent' "
                             f"/usr/lib/systemd/system/bluechi-agent.service")
        self.client.exec_run("mkdir -p /var/log/valgrind")
        self.systemctl.daemon_reload()

    def run_python(self, python_script_path: str) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        target_file_dir = os.path.join("/", "tmp")
        target_file_name = get_random_name(10)
        content = read_file(python_script_path)
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

    def cleanup(self):
        if isinstance(self.client, ContainerClient):
            if self.client.container.status == 'running':
                kw_params = {'timeout': 0}
                self.client.container.stop(**kw_params)
            self.client.container.remove()
        elif isinstance(self.client, SSHClient):
            # TODO: implement proper cleanup (removing all added files etc.)
            pass

    def gather_valgrind_logs(self, data_dir: str) -> None:
        bluechi_controller_valgrind_filename = f"bluechi-controller-valgrind-{self.name}.log"
        bluechi_agent_valgrind_filename = f"bluechi-agent-valgrind-{self.name}.log"
        bluechi_controller_valgrind_log_target_path = f"/tmp/{bluechi_controller_valgrind_filename}"
        bluechi_agent_valgrind_log_target_path = f"/tmp/{bluechi_agent_valgrind_filename}"

        # Collect valgrind logs by copying log files to the data directory
        result, _ = self.client.exec_run(
            f'cp -f {BluechiMachine.valgrind_log_path_controller} {bluechi_controller_valgrind_log_target_path}')
        if result == 0:
            self.client.get_file(bluechi_controller_valgrind_log_target_path, data_dir)
        result, _ = self.client.exec_run(
            f'cp -f {BluechiMachine.valgrind_log_path_agent} {bluechi_agent_valgrind_log_target_path}')
        if result == 0:
            self.client.get_file(bluechi_agent_valgrind_log_target_path, data_dir)

    def gather_journal_logs(self, data_dir: str) -> None:
        log_file = f"/tmp/journal-{self.name}.log"

        self.client.exec_run(
            f'bash -c "journalctl --no-pager > {log_file}"', tty=True)

        self.client.get_file(log_file, data_dir)

    def gather_coverage(self, data_coverage_dir: str) -> None:
        gcda_file_location = "/var/tmp/bluechi-coverage"
        coverage_file = f"{gcda_file_location}/coverage-{self.name}.info"

        LOGGER.info("Generating info file started")
        result, output = self.client.exec_run(
            f"/usr/share/bluechi-coverage/bin/gather-code-coverage.sh {coverage_file}")
        if result != 0:
            LOGGER.error(f"Failed to gather code coverage: {output}")
        LOGGER.info("Generating info file finished")

        self.client.get_file(f"{coverage_file}", data_coverage_dir)


class BluechiAgentMachine(BluechiMachine):

    def __init__(self, name: str, client: Client, agent_config: BluechiAgentConfig) -> None:
        super().__init__(name, client)

        self.config = agent_config

        # add confd file to container
        self.client.create_file(self.config.get_confd_dir(), self.config.file_name, self.config.serialize())


class BluechiControllerMachine(BluechiMachine):

    def __init__(self, name: str, client: Client, ctrl_config: BluechiControllerConfig) -> None:
        super().__init__(name, client)

        self.bluechictl = BluechiCtl(client)

        self.config = ctrl_config

        # add confd file to container
        self.client.create_file(self.config.get_confd_dir(), self.config.file_name, self.config.serialize())
