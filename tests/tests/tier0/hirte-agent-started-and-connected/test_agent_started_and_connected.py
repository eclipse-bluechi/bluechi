# SPDX-License-Identifier: GPL-2.0-or-later

import platform
import pytest
import socket

from hirte_tests.gather import journal
from hirte_tests.provision import containers

@pytest.fixture(autouse=True)
def handle_env(
    hirte_controller_ctr,
    hirte_node_foo_ctr,
    tmt_test_data_dir,
):
    # initialize configuration
    containers.put_files(hirte_controller_ctr, 'controller.conf.d', '/etc/hirte/hirte.conf.d')
    containers.put_files(hirte_node_foo_ctr, 'node-foo.conf.d', '/etc/hirte/agent.conf.d')

    # TODO: find out a better way how to set ManagerHost
    hirte_node_foo_ctr.exec_run(
        'sed -i "s/@MANAGER_HOST@/{manager_host}/" /etc/hirte/agent.conf.d/03-manager.conf'.format(
            manager_host=socket.gethostbyname(platform.node()),
        )
    )

    # start services
    hirte_controller_ctr.exec_run('systemctl start hirte', detach=True)
    hirte_node_foo_ctr.exec_run('systemctl start hirte-agent', detach=True)

    yield

    # gather journal from containers
    journal.gather_containers_journal(
        tmt_test_data_dir,
        hirte_controller_ctr,
        hirte_node_foo_ctr,
    )

    # clean up containers
    containers.clean_up_container(hirte_node_foo_ctr)
    containers.clean_up_container(hirte_controller_ctr)


def test_agent_foo_startup(hirte_node_foo_ctr):
    result,output = containers.exec_run(hirte_node_foo_ctr, 'systemctl is-active hirte-agent')

    assert result == 0
    assert output == 'active'

    # TODO: Add code to test that agent on node foo is successfully connected to hirte controller
