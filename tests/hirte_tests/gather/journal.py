# SPDX-License-Identifier: GPL-2.0-or-later

from hirte_tests.provision import containers


def gather_container_journal(container, data_dir):
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


def gather_containers_journal(data_dir, *args):
    for container in args:
        gather_container_journal(container, data_dir)
