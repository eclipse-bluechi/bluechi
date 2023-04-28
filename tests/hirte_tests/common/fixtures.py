# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest

from podman import PodmanClient
from podman.domain.containers import Container


@pytest.fixture(scope='session')
def hirte_image_name() -> str:
    """Returns the name of hirte testing container images"""

    return _get_env_value('HIRTE_IMAGE_NAME', 'hirte-image')


@pytest.fixture(scope='session')
def hirte_controller() -> str:
    """Returns the name of hirte controller container"""

    return _get_env_value('HIRTE_CONTROLLER', 'hirte-controller')


@pytest.fixture(scope='session')
def hirte_node_foo() -> str:
    """Returns the name of hirte node foo container"""

    return _get_env_value('HIRTE_NODE_FOO', 'hirte-node-foo')


@pytest.fixture(scope='session')
def hirte_node_bar() -> str:
    """Returns the name of hirte node bar container"""

    return _get_env_value('HIRTE_NODE_BAR', 'hirte-node-bar')


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
                filters='reference=*{image_name}'.format(image_name=hirte_image_name)
            )
        ),
        None,
    )
    return image.id if image else None


@pytest.fixture(scope='session')
def hirte_controller_ctr(
    podman_client: PodmanClient,
    hirte_image_id: str,
    hirte_controller: str,
    hirte_network_name: str,
    hirte_ctrl_host_port: str,
    hirte_ctrl_svc_port: str
) -> Container:
    """Runs hirte controller container if not running and returns its instance"""
    return podman_client.containers.run(
        name=hirte_controller,
        image=hirte_image_id,
        detach=True,
        networks={hirte_network_name: {}},
        ports={hirte_ctrl_svc_port: hirte_ctrl_host_port},
    )


@pytest.fixture(scope='session')
def hirte_node_foo_ctr(
        podman_client: PodmanClient,
        hirte_image_id: str,
        hirte_node_foo: str,
        hirte_network_name: str) -> Container:
    """Runs hirte node foo container and returns its instance"""
    return podman_client.containers.run(
        name=hirte_node_foo,
        image=hirte_image_id,
        detach=True,
        networks={hirte_network_name: {}},
    )


@pytest.fixture(scope='session')
def hirte_node_bar_ctr(
        podman_client: PodmanClient,
        hirte_image_id: str,
        hirte_node_bar: str,
        hirte_network_name: str) -> Container:
    """Runs hirte node bar container and returns its instance"""
    return podman_client.containers.run(
        name=hirte_node_bar,
        image=hirte_image_id,
        detach=True,
        networks={hirte_network_name: {}},
    )


@pytest.fixture(scope='function')
def tmt_test_data_dir() -> str:
    """Return directory, where tmt saves data of the relevant test. If the TMT_TEST_DATA env variable is not set, then
    use current directory"""
    return _get_env_value('TMT_TEST_DATA', os.getcwd())
