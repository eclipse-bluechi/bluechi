# SPDX-License-Identifier: GPL-2.0-or-later

import io
import os
import tarfile

def clean_up_all_containers(hirte_controller_ctr, hirte_node_foo_ctr, hirte_node_bar_ctr):
    clean_up_container(hirte_node_bar_ctr)
    clean_up_container(hirte_node_foo_ctr)
    clean_up_container(hirte_controller_ctr)


def clean_up_container(container):
    if container.status == 'running':
        container.stop()
    container.remove()


def put_file(container, local_file, container_directory):
    content = None
    with open(local_file, 'rb') as fh:
        content = io.BytesIO(fh.read())

    buff: IO[bytes] = io.BytesIO()

    with tarfile.open(fileobj=buff, mode="w:") as file:
        info: tarfile.TarInfo = tarfile.TarInfo()
        info.uid = 0
        info.gid = 0
        info.name = os.path.basename(local_file)
        info.path = os.path.basename(local_file)
        info.mode = 0o644
        info.type = tarfile.REGTYPE
        info.size = len(content.getvalue())
        file.addfile(info, fileobj=io.BytesIO(content.getvalue()))

    buff.seek(0)
    container.put_archive(path=container_directory, data=buff)


def put_files(container, local_directory, container_directory):
    list_of_files = sorted(
        filter(lambda x: os.path.isfile(os.path.join(local_directory, x)), os.listdir(local_directory))
    )
    for file_name in list_of_files:
        put_file(container, os.path.join(local_directory, file_name), container_directory)


def get_file(container, container_path, local_path):
    actual, stats = container.get_archive(container_path)

    with io.BytesIO() as fd:
        for chunk in actual:
            fd.write(chunk)
        fd.seek(0, 0)

        with tarfile.open(fileobj=fd, mode="r") as tar:
            tar.extractall(path=local_path)


def exec_run(container, command, raw_output=False):
    """Executes command inside the specified container and returns result code and processed output"""
    result, output = container.exec_run(command)

    if not raw_output and output:
        output = output.decode('utf-8').strip()

    return result, output
