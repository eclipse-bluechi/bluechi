# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import pytest
import yaml

from podman import PodmanClient
from typing import Union, List

from bluechi_test.test import BlueChiTest, BlueChiContainerTest, BlueChiSSHTest, AvailableHost
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig
from bluechi_test.util import get_primary_ip


def _get_env_value(env_var: str, default_value: str) -> str:
    value = os.getenv(env_var)
    if value is None:
        return default_value
    return value

# TMT specific fixtures


@pytest.fixture(scope='session')
def tmt_test_data_dir() -> str:
    """Return directory, where tmt saves data of the relevant test. If the TMT_TEST_DATA env variable is not set, then
    use current directory"""
    return _get_env_value('TMT_TEST_DATA', os.getcwd())


@pytest.fixture(scope='function')
def tmt_test_serial_number() -> str:
    """Return serial number of current test"""
    return _get_env_value('TMT_TEST_SERIAL_NUMBER', 'NA')


# General fixtures

@pytest.fixture(scope='session')
def run_with_valgrind() -> bool:
    """Returns 1 if bluechi should be run with valgrind for memory management testing"""
    print()
    print()
    print(f"WITH_VALGRIND: {_get_env_value('WITH_VALGRIND', 0)}")
    print()
    print()
    return _get_env_value('WITH_VALGRIND', 0) == '1'


@pytest.fixture(scope='session')
def run_with_coverage() -> bool:
    """Returns 1 if code coverage should be collected"""

    return _get_env_value('WITH_COVERAGE', 0) == '1'


@pytest.fixture(scope='function')
def bluechi_ctrl_default_config(bluechi_ctrl_svc_port: str):
    return BlueChiControllerConfig(file_name="ctrl.conf", port=bluechi_ctrl_svc_port)


@pytest.fixture(scope='function')
def bluechi_node_default_config(bluechi_ctrl_svc_port: str):
    return BlueChiAgentConfig(
        file_name="agent.conf",
        controller_host=get_primary_ip(),
        controller_port=bluechi_ctrl_svc_port)


# Container specific fixtures

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
def additional_ports():
    return None


# SSH specific fixtures

@pytest.fixture(scope='session')
def available_hosts() -> List[AvailableHost]:
    """ Returns a list of provisioned machines to connect to via ssh """
    hosts: List[AvailableHost] = []
    tmt_dir = _get_env_value("TMT_TREE", "")
    fmf_file = _get_env_value("FMF_FILE_NAME", "")
    if fmf_file == "":
        return hosts
    with open(os.path.join(tmt_dir, "plans", fmf_file), "r") as f:
        y = yaml.safe_load(f.read())
        for machine in y["provision"]:
            if machine["how"] == "connect":
                hosts.append(AvailableHost(
                    host=machine["guest"],
                    port=int(machine["port"]),
                    user=machine["user"],
                    password="" if "password" not in machine else machine["password"],
                    key="" if "key" not in machine else machine["key"],
                ))
    return hosts


@pytest.fixture(scope='session')
def is_container_test(available_hosts: List[AvailableHost]) -> bool:
    """ Returns if the current run is executed via containers or on machines using ssh """
    return len(available_hosts) == 0


@pytest.fixture(scope='function')
def bluechi_test(
        is_container_test: bool,
        available_hosts: List[AvailableHost],
        podman_client: PodmanClient,
        bluechi_image_id: str,
        bluechi_ctrl_host_port: str,
        bluechi_ctrl_svc_port: str,
        tmt_test_serial_number: str,
        tmt_test_data_dir: str,
        run_with_valgrind: bool,
        run_with_coverage: bool,
        additional_ports: dict) -> BlueChiTest:

    if is_container_test:
        return BlueChiContainerTest(
            podman_client,
            bluechi_image_id,
            bluechi_ctrl_host_port,
            bluechi_ctrl_svc_port,
            tmt_test_serial_number,
            tmt_test_data_dir,
            run_with_valgrind,
            run_with_coverage,
            additional_ports,
        )

    return BlueChiSSHTest(
        available_hosts,
        bluechi_ctrl_svc_port,
        tmt_test_serial_number,
        tmt_test_data_dir,
        run_with_valgrind,
        run_with_coverage,
    )
