# SPDX-License-Identifier: LGPL-2.1-or-later

import io
import logging
import os
import time
import tarfile

from podman.domain.containers import Container
from typing import Any, Iterator, Optional, Tuple, Union, IO

from bluechi_test.config import BluechiConfig, BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.util import read_file, get_random_name

LOGGER = logging.getLogger(__name__)

valgrind_log_path_controller = '/var/log/valgrind/bluechi-controller-valgrind.log'
valgrind_log_path_agent = '/var/log/valgrind/bluechi-agent-valgrind.log'


class BluechiMachine():

    def __init__(self, container: Container, config: BluechiConfig) -> None:
        self.container: Container = container

        # add confd file to container
        self.create_file(config.get_confd_dir(), config.file_name, config.serialize())

    def create_file(self, target_dir: str, file_name: str, content: str) -> None:
        content_encoded = content.encode("utf-8")

        buff: IO[bytes] = io.BytesIO()
        with tarfile.open(fileobj=buff, mode="w:") as file:
            info: tarfile.TarInfo = tarfile.TarInfo()
            info.uid = 0
            info.gid = 0
            info.name = os.path.basename(file_name)
            info.path = os.path.basename(file_name)
            info.mode = 0o644
            info.type = tarfile.REGTYPE
            info.size = len(content_encoded)
            file.addfile(info, fileobj=io.BytesIO(content_encoded))

        buff.seek(0)
        self.container.put_archive(path=target_dir, data=buff)

    def get_file(self, container_path: str, local_path: str) -> None:
        actual, _ = self.container.get_archive(container_path)

        with io.BytesIO() as fd:
            for chunk in actual:
                fd.write(chunk)
            fd.seek(0, 0)

            with tarfile.open(fileobj=fd, mode="r") as tar:
                tar.extractall(path=local_path)

    def gather_journal_logs(self, data_dir: str) -> None:
        log_file = f"/tmp/journal-{self.container.name}.log"

        self.container.exec_run(
            f'bash -c "journalctl --no-pager > {log_file}"', tty=True)

        self.get_file(log_file, data_dir)

    def gather_coverage(self, data_coverage_dir: str) -> None:
        gcda_file_location = "/var/tmp/bluechi-coverage"
        coverage_file = f"{gcda_file_location}/coverage-{self.container.name}.info"

        LOGGER.info("Generating info file started")
        result, output = self.exec_run(f"/usr/share/bluechi-coverage/bin/gather-code-coverage.sh {coverage_file}")
        LOGGER.info("Generating info file finished")

        self.get_file(f"{coverage_file}", data_coverage_dir)

    def cleanup(self):
        if self.container.status == 'running':
            kw_params = {'timeout': 0}
            self.container.stop(**kw_params)
        self.container.remove()

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False, tty: bool = True) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.container.exec_run(command, tty=tty)
        LOGGER.debug(f"Executed command '{command}' with result '{result}' and output '{output}'")

        if not raw_output and output:
            output = output.decode('utf-8').strip()

        return result, output

    def systemctl_daemon_reload(self) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.exec_run("systemctl daemon-reload")

    def systemctl_start_and_wait(self, service_name: str, sleep_after: float = 0.0) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]], Optional[bool]]:

        result, output = self.exec_run(f"systemctl start {service_name}")
        wait_result = self.wait_for_unit_state_to_be(service_name, "active")
        time.sleep(sleep_after)

        return result, output, wait_result

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        source_path = os.path.join(source_dir, service_file_name)
        content = read_file(source_path)

        LOGGER.debug(f"Copy local systemd service '{source_path}' to container path '{target_dir}'\
             with content:\n{content}")
        self.create_file(target_dir, service_file_name, content)
        self.systemctl_daemon_reload()

    def copy_container_script(self, script_file_name: str):
        curr_dir = os.getcwd()
        source_path = os.path.join(curr_dir, "..", "..", "..", "bluechi_test", "container_scripts", script_file_name)
        content = read_file(source_path)

        target_dir = os.path.join("/", "var")

        LOGGER.info(f"Copy container script '{source_path}' to container path '{curr_dir}'\
             with content:\n{content}")
        self.create_file(target_dir, script_file_name, content)

    def service_is_active(self, unit_name: str) -> bool:
        result, _ = self.exec_run(f"systemctl is-active {unit_name}")
        return result == 0

    def get_unit_freezer_state(self, unit_name: str) -> str:
        _, output = self.exec_run(f"systemctl show {unit_name} --property=FreezerState")
        state = str(output).split('=', 1)
        result = ""
        if len(state) > 1:
            result = state[1]
        return result

    def get_unit_state(self, unit_name: str) -> str:
        _, output = self.exec_run(f"systemctl is-active {unit_name}")
        return output

    def _is_unit_in_state(self, unit_name: str, expected_state: str) -> bool:
        latest_state = self.get_unit_state(unit_name)
        LOGGER.info(f"Got state '{latest_state}' for unit {unit_name}")
        return latest_state == expected_state

    def wait_for_unit_state_to_be(
            self,
            unit_name: str,
            expected_state: str,
            timeout: float = 5.0,
            delay: float = 0.5) -> bool:

        if self._is_unit_in_state(unit_name, expected_state):
            return True

        start = time.time()
        while (time.time() - start) < timeout:
            time.sleep(delay)
            if self._is_unit_in_state(unit_name, expected_state):
                return True

        LOGGER.error(f"Timeout while waiting for '{unit_name}' to reach state '{expected_state}'.")
        return False

    def enable_valgrind(self) -> None:
        self.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                      f"--log-file={valgrind_log_path_controller} /usr/libexec/bluechi-controller' "
                      f"/usr/lib/systemd/system/bluechi-controller.service")
        self.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/bin/valgrind -s --leak-check=yes "
                      f"--log-file={valgrind_log_path_agent} /usr/libexec/bluechi-agent' "
                      f"/usr/lib/systemd/system/bluechi-agent.service")
        self.exec_run("mkdir -p /var/log/valgrind")
        self.exec_run("systemctl daemon-reload")

    def run_python(self, python_script_path: str) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        target_file_dir = os.path.join("/", "tmp")
        target_file_name = get_random_name(10)
        content = read_file(python_script_path)
        self.create_file(target_file_dir, target_file_name, content)

        target_file_path = os.path.join(target_file_dir, target_file_name)
        LOGGER.debug(f"Executing python script '{target_file_path}'")
        result, output = self.exec_run(f'python3 {target_file_path}')
        LOGGER.debug(f"Execute python script '{target_file_path}' finished with result '{result}' \
            and output:\n{output}")
        try:
            os.remove(target_file_path)
        finally:
            return result, output

    def extract_valgrind_logs(self, log_path: str, target_path: str, data_dir: str) -> None:
        result, _ = self.exec_run(f'cp -f {log_path} {target_path}')
        if result == 0:
            self.get_file(target_path, data_dir)

    def gather_valgrind_logs(self, data_dir: str) -> None:
        bluechi_valgrind_filename = f"bluechi-controller-valgrind-{self.container.name}.log"
        bluechi_agent_valgrind_filename = f"bluechi-agent-valgrind-{self.container.name}.log"
        bluechi_valgrind_log_target_path = f"/tmp/{bluechi_valgrind_filename}"
        bluechi_agent_valgrind_log_target_path = f"/tmp/{bluechi_agent_valgrind_filename}"

        # Collect valgrind logs to the data directory
        self.extract_valgrind_logs(valgrind_log_path_controller, bluechi_valgrind_log_target_path, data_dir)
        self.extract_valgrind_logs(valgrind_log_path_agent, bluechi_agent_valgrind_log_target_path, data_dir)

    def restart_with_config_file(self, config_file_location, service):
        self.exec_run(f"sed -i '/ExecStart=/c\\ExecStart=/usr/libexec/{service} -c "
                      f"{config_file_location}' "
                      f"/usr/lib/systemd/system/{service}.service")
        self.exec_run("systemctl daemon-reload")
        self.exec_run(f"systemctl restart {service}.service")


class BluechiAgentMachine(BluechiMachine):

    def __init__(self, container: Container, config: BluechiAgentConfig) -> None:
        super().__init__(container, config)

        self.config = config
        self.node_name = config.node_name

    def wait_for_bluechi_agent(self):
        should_wait = True
        while should_wait:
            should_wait = not self.service_is_active("bluechi-agent")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_bluechi_agent()


class BluechiControllerMachine(BluechiMachine):

    def __init__(self, container: Container, config: BluechiControllerConfig) -> None:
        super().__init__(container, config)

        self.config = config

    def wait_for_bluechi(self):
        should_wait = True
        while should_wait:
            should_wait = not self.service_is_active("bluechi-controller")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_bluechi()

    def start_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Starting unit '{unit_name}' on node '{node_name}'")

        result, output = self.exec_run(f"bluechictl start {node_name} {unit_name}")
        LOGGER.debug(f"Start unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def stop_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Stopping unit '{unit_name}' on node '{node_name}'")

        result, output = self.exec_run(f"bluechictl stop {node_name} {unit_name}")
        LOGGER.debug(f"Stop unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def enable_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Enabling unit '{unit_name}' on node '{node_name}'")

        result, output = self.exec_run(f"bluechictl enable {node_name} {unit_name}")
        LOGGER.debug(f"Enable unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to enable service {unit_name} on node {node_name}: {output}")

    def freeze_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Freezing unit {unit_name} on node {node_name}")

        result, output = self.exec_run(f"bluechictl freeze {node_name} {unit_name}")
        LOGGER.debug(f"Freeze unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to freeze service {unit_name} on node {node_name}: {output}")

    def thaw_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Thawing unit {unit_name} on node {node_name}")

        result, output = self.exec_run(f"bluechictl thaw {node_name} {unit_name}")
        LOGGER.debug(f"Thaw unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to thaw service {unit_name} on node {node_name}: {output}")
