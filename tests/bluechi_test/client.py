# SPDX-License-Identifier: LGPL-2.1-or-later

import io
import logging
import os
import pathlib
import tarfile

from podman import PodmanClient
from podman.domain.containers import Container
from paramiko import SSHClient as ParamikoSSH, AutoAddPolicy, RSAKey, SFTP
from typing import Any, Dict, Iterator, Optional, Tuple, Union, IO

LOGGER = logging.getLogger(__name__)


class Client():

    def __init__(self) -> None:
        pass

    def create_file(self, target_dir: str, file_name: str, content: str) -> None:
        raise Exception("Not implemented!")

    def get_file(self, machine_path: str, local_path: str) -> None:
        raise Exception("Not implemented!")

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False, tty: bool = True) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        raise Exception("Not implemented!")


class ContainerClient(Client):

    def __init__(self, podman_client: PodmanClient, image_id: str, name: str, ports: Dict[str, str]) -> None:
        super().__init__()

        self.container: Container = podman_client.containers.run(
            name=name,
            image=image_id,
            detach=True,
            ports=ports,
        )
        self.container.wait(condition="running")

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

    def get_file(self, machine_path: str, local_path: str) -> None:
        actual, _ = self.container.get_archive(machine_path)

        with io.BytesIO() as fd:
            for chunk in actual:
                fd.write(chunk)
            fd.seek(0, 0)

            with tarfile.open(fileobj=fd, mode="r") as tar:
                tar.extractall(path=local_path)

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False, tty: bool = True) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.container.exec_run(command, tty=tty)
        LOGGER.debug(f"Executed command '{command}' with result '{result}' and output '{output}'")

        if not raw_output and output:
            output = output.decode('utf-8').strip()

        return result, output


class SSHClient(Client):

    def __init__(self, host: str, user: str, port: int, key_path: str, password: str = "") -> None:
        self.ssh = ParamikoSSH()
        self.ssh.set_missing_host_key_policy(AutoAddPolicy())

        if password != "":
            self.ssh.connect(hostname=host, port=port, username=user, password=password)
        else:
            self.ssh.connect(hostname=host,
                             port=port,
                             username=user,
                             pkey=RSAKey.from_private_key_file(key_path),
                             passphrase="",
                             look_for_keys=False)

    def create_file(self, target_dir: str, file_name: str, content: str) -> None:
        ftp: SFTP = None
        try:
            ftp = self.ssh.open_sftp()
            ftp.putfo(io.BytesIO(content.encode()), os.path.join(target_dir, file_name))
        finally:
            if ftp is not None:
                ftp.close()

    def get_file(self, machine_path: str, local_path: str) -> None:
        local_file_name = pathlib.Path(machine_path).name
        ftp: SFTP = None
        try:
            ftp = self.ssh.open_sftp()
            file = ftp.file(machine_path, "r", -1)
            with open(os.path.join(local_path, local_file_name), "w") as f:
                f.write(file.read().decode("utf-8"))
                f.flush()
        finally:
            if ftp is not None:
                ftp.close()

    def exec_run(self, command: (Union[str, list[str]]), raw_output: bool = False, tty: bool = True) -> \
            Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        stdin, stdout, stderr = None, None, None
        try:
            stdin, stdout, stderr = self.ssh.exec_command(command)

            result = stdout.channel.recv_exit_status()
            output = stdout.read()
            err = stderr.read().decode("utf8")

            LOGGER.debug(f"Executed command '{command}' with result '{result}' and output '{output}' and error '{err}'")

            if not raw_output and output:
                output = output.decode('utf-8').strip()

            return result, output
        finally:
            if stdin is not None:
                stdin.close()
            if stdout is not None:
                stdout.close()
            if stderr is not None:
                stderr.close()

    def close(self):
        self.ssh.close()
