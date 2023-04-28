# SPDX-License-Identifier: GPL-2.0-or-later

import platform
import pytest

from hirte_tests.gather import journal
from hirte_tests.provision import containers


@pytest.fixture(autouse=True)
def handle_env(
    hirte_controller_ctr,
    tmt_test_data_dir,
):
    # initialize hirte configuration
    containers.put_files(hirte_controller_ctr, 'controller.conf.d', '/etc/hirte/hirte.conf.d')

    # start services
    hirte_controller_ctr.exec_run('systemctl start hirte', detach=True)

    yield

    # gather journal from containers
    journal.gather_containers_journal(
        tmt_test_data_dir,
        hirte_controller_ctr,
    )

    # clean up containers
    containers.clean_up_container(hirte_controller_ctr)


def test_controller_startup(hirte_controller_ctr):
    result, output = containers.exec_run(hirte_controller_ctr, 'systemctl is-active hirte')

    assert result == 0
    assert output == 'active'
