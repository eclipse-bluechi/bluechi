# SPDX-License-Identifier: GPL-2.0-or-later

import io
import os
import time
import tarfile

from podman import PodmanClient
from podman.domain.containers import Container
from typing import Any, Iterator, Optional, Tuple, Union, IO

from bluechi_test.config import BluechiConfig, BluechiNodeConfig, BluechiControllerConfig
from bluechi_test.util import read_file


class BluechiContainer():

    def __init__(self, container: Container, config: BluechiConfig) -> None:
        self.container: Container = container
        self.config: BluechiConfig = config

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

    def cleanup(self):
        if self.container.status == 'running':
            self.container.stop()
        self.container.remove()

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.container.exec_run(command)

        if not raw_output and output:
            output = output.decode('utf-8').strip()

        return result, output

    def list_containers(self) -> str:
        container_ip = None

        with PodmanClient() as client:
            container = client.containers.list()
            if container:
                for i in container:
                    print(i.name)

        return container_ip

    def get_ip(self) -> str:
        container_ip = None

        print("==== DOUG entering PodManClient")
        with PodmanClient() as client:
            container = client.containers.list(filters={"name": "bluechi-controller"})
            print("===== DOUG container =====")
            print(container)
            print("===== DOUG container =====")
            if container:
                print("===== DOUG entrei container - container_data =====")
                container_data = container[0]
                print(container_data)
                print("===== DOUG entrei container - container_data =====")
                print("===== DOUG entrei container - container_data.inspect =====")
                container_inspect = container_data.inspect()
                print(container_inspect)
                print("===== DOUG entrei container - container_data.inspect =====")
                print("===== DOUG entrei container - container_ip =====")
                container_ip = container_inspect["NetworkSettings"]["IPAddress"]
                print(container_ip)
                print("===== DOUG entrei container - container_ip =====")

        return container_ip

    def set_config(
        self,
        option_name: str,
        option_value: str,
        config_path: str
    ):
        result = None
        cmd = f"sed -i 's/^{option_name}=.*/{option_name}={option_value}/' {config_path}"

        result, _ = self.container.exec_run(cmd)
        return result

    def systemctl_daemon_reload(self) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.exec_run("systemctl daemon-reload")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        source_path = os.path.join(source_dir, service_file_name)
        content = read_file(source_path)

        print(f"Copy local systemd service '{source_path}' to container path '{target_dir}' with content:")
        print(f"{content}\n")
        self.create_file(target_dir, service_file_name, content)
        self.systemctl_daemon_reload()

    def service_is_active(self, unit_name: str) -> bool:
        result, _ = self.exec_run(f"systemctl is-active {unit_name}")
        return result == 0

    def get_unit_state(self, unit_name: str) -> str:
        _, output = self.exec_run(f"systemctl is-active {unit_name}")
        return output

    def wait_for_unit_state_to_be(self, unit_name: str, expected_state: str, timeout: float = 2.0) -> bool:
        latest_state = ""

        start = time.time()
        while (time.time() - start) < timeout:
            latest_state = self.get_unit_state(unit_name)
            if latest_state == expected_state:
                return True

        print(f"Timeout while waiting for {unit_name} to reach state {expected_state}. \
              Latest state: {latest_state}")
        return False


class BluechiNodeContainer(BluechiContainer):

    def __init__(self, container: Container, config: BluechiNodeConfig) -> None:
        super().__init__(container, config)
        self.node_name = config.node_name

    def wait_for_bluechi_agent(self):
        should_wait = True
        while should_wait:
            should_wait = not self.service_is_active("bluechi-agent")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_bluechi_agent()

    def get_ip_node(self, node_name: str) -> str:
        container_ip = None

        with PodmanClient() as client:
            container = client.containers.list(filters={"name": node_name})

            if container:
                container_data = container[0]
                container_inspect = container_data.inspect()
                container_ip = container_inspect["NetworkSettings"]["IPAddress"]

        return container_ip


class BluechiControllerContainer(BluechiContainer):

    def __init__(self, container: Container, config: BluechiControllerConfig) -> None:
        super().__init__(container, config)

    def wait_for_bluechi(self):
        should_wait = True
        while should_wait:
            should_wait = not self.service_is_active("bluechi")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_bluechi()

    def start_unit(self, node_name: str, unit_name: str) -> None:
        print(f"Starting unit {unit_name} on node {node_name}")

        result, output = self.exec_run(f"bluechictl start {node_name} {unit_name}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def stop_unit(self, node_name: str, unit_name: str) -> None:
        print(f"Stopping unit {unit_name} on node {node_name}")

        result, output = self.exec_run(f"bluechictl stop {node_name} {unit_name}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def enable_unit(self, node_name: str, unit_name: str) -> None:
        print(f"Enabling unit {unit_name} on node {node_name}")

        result, output = self.exec_run(f"bluechictl enable {node_name} {unit_name}")
        if result != 0:
            raise Exception(f"Failed to enable service {unit_name} on node {node_name}: {output}")
