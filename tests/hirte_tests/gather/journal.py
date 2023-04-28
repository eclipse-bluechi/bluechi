# SPDX-License-Identifier: GPL-2.0-or-later

from typing import List
from podman.domain.containers import Container

from hirte_tests.provision import containers


def gather_container_journal(data_dir: str, container: Container) -> None:
    log_file = '/tmp/journal-{ctr_name}.log'.format(
        ctr_name=container.name,
    )
    container.exec_run(
        'bash -c "journalctl --no-pager > {log_file}"'.format(
            log_file=log_file,
        ),
        tty=True,
    )
    containers.get_file(container, log_file, data_dir)


def gather_containers_journal(data_dir: str, containers: List[Container]) -> None:
    for container in containers:
        gather_container_journal(data_dir, container)
