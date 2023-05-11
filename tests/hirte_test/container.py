# SPDX-License-Identifier: GPL-2.0-or-later

import io
import os
import tarfile

from podman.domain.containers import Container
from typing import Any, Iterator, Optional, Tuple, Union, IO

from hirte_test.config import HirteConfig, HirteNodeConfig, HirteControllerConfig
from hirte_test.util import read_file


class HirteContainer():

    def __init__(self, container: Container, config: HirteConfig) -> None:
        self.container: Container = container
        self.config: HirteConfig = config

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

    def exec_run(self, command: (str | list[str]), raw_output: bool = False) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.container.exec_run(command)

        if not raw_output and output:
            output = output.decode('utf-8').strip()

        return result, output

    def systemctl_daemon_reload(self) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.exec_run("systemctl daemon-reload")

    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        source_path = os.path.join(source_dir, service_file_name)
        target_path = os.path.join(target_dir, service_file_name)

        content = read_file(source_path)

        print(f"Copy local systemd service '{source_path}' to container path '{target_dir}' with content:")
        print(f"{content}\n")
        self.create_file(target_path, service_file_name, content)
        self.systemctl_daemon_reload()


class HirteControllerContainer(HirteContainer):

    def __init__(self, container: Container, config: HirteControllerConfig) -> None:
        super().__init__(container, config)

    def wait_for_hirte(self):
        result = 1
        while result != 0:
            result, _ = self.exec_run("systemctl is-active hirte")
    
    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_hirte()


class HirteNodeContainer(HirteContainer):

    def __init__(self, container: Container, config: HirteNodeConfig) -> None:
        super().__init__(container, config)

    def wait_for_hirte_agent(self):
        result = 1
        while result != 0:
            result, _ = self.exec_run("systemctl is-active hirte-agent")
    
    def copy_systemd_service(self, service_file_name: str, source_dir: str, target_dir):
        super().copy_systemd_service(service_file_name, source_dir, target_dir)
        self.wait_for_hirte_agent()
