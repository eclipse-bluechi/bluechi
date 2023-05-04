# SPDX-License-Identifier: GPL-2.0-or-later

import os
import platform
import pytest
import socket

from podman import PodmanClient

from tests.lib.test import HirteTest
from tests.lib.config import HirteControllerConfig, HirteNodeConfig


@pytest.fixture(scope='session')
def tmt_test_data_dir() -> str:
    """Return directory, where tmt saves data of the relevant test. If the TMT_TEST_DATA env variable is not set, then
    use current directory"""
    return _get_env_value('TMT_TEST_DATA', os.getcwd())


@pytest.fixture(scope='session')
def hirte_image_name() -> str:
    """Returns the name of hirte testing container images"""

    return _get_env_value('HIRTE_IMAGE_NAME', 'hirte-image')


@pytest.fixture(scope='session')
def hirte_ctrl_host_port() -> str:
    """Returns the port, which hirte controller service is mapped to on a host"""

    return _get_env_value('HIRTE_CTRL_HOST_PORT', '8420')


@pytest.fixture(scope='session')
def hirte_ctrl_svc_port() -> str:
    """Returns the port, which hirte controller service is using inside a container"""

    return _get_env_value('HIRTE_CTRL_SVC_PORT', '8420')


@pytest.fixture(scope='session')
def hirte_network_name() -> str:
    """Returns the name of the network used for testing containers"""

    return _get_env_value('HIRTE_NETWORK_NAME', 'hirte-test')


def _get_env_value(env_var: str, default_value: str) -> str:
    value = os.getenv(env_var)
    if value is None:
        return default_value
    return value


@pytest.fixture(scope='session')
def podman_client() -> PodmanClient:
    """Returns the podman client instance"""
    return PodmanClient()


@pytest.fixture(scope='session')
def hirte_image_id(podman_client: PodmanClient, hirte_image_name: str) -> (str | None):
    """Returns the image ID of hirte testing containers"""
    image = next(
        iter(
            podman_client.images.list(
                filters='reference=*{image_name}'.format(
                    image_name=hirte_image_name)
            )
        ),
        None,
    )
    return image.id if image else None


@pytest.fixture(scope='function')
def hirte_ctrl_default_config(hirte_ctrl_svc_port: str):
    return HirteControllerConfig(file_name="ctrl.conf", port=hirte_ctrl_svc_port)


@pytest.fixture(scope='function')
def hirte_node_default_config(hirte_ctrl_svc_port: str):
    return HirteNodeConfig(
        file_name="agent.conf",
        manager_host=socket.gethostbyname(platform.node()),
        manager_port=hirte_ctrl_svc_port)


@pytest.fixture(scope='function')
def hirte_test(
        podman_client: PodmanClient,
        hirte_image_id: str,
        hirte_network_name: str,
        hirte_ctrl_host_port: str,
        hirte_ctrl_svc_port: str,
        tmt_test_data_dir: str):

    return HirteTest(
        podman_client,
        hirte_image_id,
        hirte_network_name,
        hirte_ctrl_host_port,
        hirte_ctrl_svc_port,
        tmt_test_data_dir,
    )
