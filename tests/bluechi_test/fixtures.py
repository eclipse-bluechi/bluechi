# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import pytest

from podman import PodmanClient
from typing import Union

from bluechi_test.test import BluechiTest
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig
from bluechi_test.util import get_primary_ip


@pytest.fixture(scope='session')
def tmt_test_data_dir() -> str:
    """Return directory, where tmt saves data of the relevant test. If the TMT_TEST_DATA env variable is not set, then
    use current directory"""
    return _get_env_value('TMT_TEST_DATA', os.getcwd())


@pytest.fixture(scope='function')
def tmt_test_serial_number() -> str:
    """Return serial number of current test"""
    return _get_env_value('TMT_TEST_SERIAL_NUMBER', 'NA')


@pytest.fixture(scope='session')
def bluechi_image_name() -> str:
    """Returns the name of bluechi testing container images"""
    return _get_env_value('BLUECHI_IMAGE_NAME', 'bluechi-image')


@pytest.fixture(scope='session')
def bluechi_ctrl_host_port() -> str:
    """Returns the port, which bluechi controller service is mapped to on a host"""

    return _get_env_value('BLUECHI_CTRL_HOST_PORT', '8420')


@pytest.fixture(scope='session')
def bluechi_ctrl_svc_port() -> str:
    """Returns the port, which bluechi controller service is using inside a container"""

    return _get_env_value('BLUECHI_CTRL_SVC_PORT', '8420')


@pytest.fixture(scope='session')
def run_with_valgrind() -> bool:
    """Returns 1 if bluechi should be run with valgrind for memory management testing"""

    return _get_env_value('WITH_VALGRIND', 0) == '1'


@pytest.fixture(scope='session')
def run_with_coverage() -> bool:
    """Returns 1 if code coverage should be collected"""

    return _get_env_value('WITH_COVERAGE', 0) == '1'


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
def bluechi_image_id(podman_client: PodmanClient, bluechi_image_name: str) -> Union[str, None]:
    """Returns the image ID of bluechi testing containers"""
    image = next(
        iter(
            podman_client.images.list(
                filters='reference=*{image_name}'.format(
                    image_name=bluechi_image_name)
            )
        ),
        None,
    )
    assert image, f"Image '{bluechi_image_name}' was not found"
    return image.id


@pytest.fixture(scope='function')
def bluechi_ctrl_default_config(bluechi_ctrl_svc_port: str):
    return BluechiControllerConfig(file_name="ctrl.conf", port=bluechi_ctrl_svc_port)


@pytest.fixture(scope='function')
def bluechi_node_default_config(bluechi_ctrl_svc_port: str):
    return BluechiNodeConfig(
        file_name="agent.conf",
        controller_host=get_primary_ip(),
        controller_port=bluechi_ctrl_svc_port)


@pytest.fixture(scope='function')
def additional_ports():
    return None


@pytest.fixture(scope='function')
def bluechi_test(
        podman_client: PodmanClient,
        bluechi_image_id: str,
        bluechi_ctrl_host_port: str,
        bluechi_ctrl_svc_port: str,
        tmt_test_serial_number: str,
        tmt_test_data_dir: str,
        run_with_valgrind: bool,
        run_with_coverage: bool,
        additional_ports: dict):

    return BluechiTest(
        podman_client,
        bluechi_image_id,
        bluechi_ctrl_host_port,
        bluechi_ctrl_svc_port,
        tmt_test_serial_number,
        tmt_test_data_dir,
        run_with_valgrind,
        run_with_coverage,
        additional_ports
    )
