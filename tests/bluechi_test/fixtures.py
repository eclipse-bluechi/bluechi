#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
import os
from typing import Any, Dict, List, Tuple, Union

import pytest
import yaml
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.test import BluechiContainerTest, BluechiSSHTest, BluechiTest
from bluechi_test.util import get_env_value, get_primary_ip, safely_parse_int
from podman import PodmanClient


@pytest.fixture(scope="session")
def tmt_test_data_dir() -> str:
    """Return directory, where tmt saves data of the relevant test. If the TMT_TEST_DATA env variable is not set, then
    use current directory"""
    return get_env_value("TMT_TEST_DATA", os.getcwd())


@pytest.fixture(scope="function")
def tmt_test_serial_number() -> str:
    """Return serial number of current test"""
    return get_env_value("TMT_TEST_SERIAL_NUMBER", "NA")


@pytest.fixture(scope="session")
def bluechi_image_name() -> str:
    """Returns the name of bluechi testing container images"""
    return get_env_value("BLUECHI_IMAGE_NAME", "bluechi-image")


@pytest.fixture(scope="session")
def bluechi_ctrl_host_port() -> str:
    """Returns the port, which bluechi controller service is mapped to on a host"""

    return get_env_value("BLUECHI_CTRL_HOST_PORT", "8420")


@pytest.fixture(scope="session")
def bluechi_ctrl_svc_port() -> str:
    """Returns the port, which bluechi controller service is using inside a container"""

    return get_env_value("BLUECHI_CTRL_SVC_PORT", "8420")


@pytest.fixture(scope="session")
def machines_ssh_user() -> str:
    """Returns the user for connecting to the available hosts via SSH"""

    return get_env_value("SSH_USER", "root")


@pytest.fixture(scope="session")
def machines_ssh_password() -> str:
    """Returns the password for connecting to the available hosts via SSH"""

    return get_env_value("SSH_PASSWORD", "")


@pytest.fixture(scope="session")
def timeout_test_setup() -> int:
    """Returns the timeout for setting up the test setup"""

    return safely_parse_int(get_env_value("TIMEOUT_TEST_SETUP", ""), 20)


@pytest.fixture(scope="session")
def timeout_test_run() -> int:
    """Returns the timeout for executing the actual test"""

    return safely_parse_int(get_env_value("TIMEOUT_TEST_RUN", ""), 45)


@pytest.fixture(scope="session")
def timeout_collecting_test_results() -> int:
    """Returns the timeout for collecting all test results"""

    return safely_parse_int(get_env_value("TIMEOUT_COLLECT_TEST_RESULTS", ""), 20)


def _read_topology() -> Dict[str, Any]:
    """
    Returns the parsed YAML for the tmt guest topology:
    https://tmt.readthedocs.io/en/stable/spec/plans.html#guest-topology-format
    """
    tmt_yaml_file = get_env_value("TMT_TOPOLOGY_YAML", "")
    if tmt_yaml_file is None or tmt_yaml_file == "":
        return get_primary_ip()

    topology = dict()
    with open(tmt_yaml_file, "r") as f:
        topology = yaml.safe_load(f.read())
    return topology


@pytest.fixture(scope="session")
def bluechi_ctrl_host_ip() -> str:
    """
    Returns the ip of the host on which bluechi controller service is running.
    It determines the controller guest by looking for the name [controller, bluechi-controller]
    in the name.
    """
    topology = _read_topology()

    if "guests" not in topology:
        return get_primary_ip()

    for _, values in topology["guests"].items():
        if values["role"] == "controller":
            return values["hostname"]

    return get_primary_ip()


@pytest.fixture(scope="session")
def available_hosts() -> Dict[str, List[Tuple[str, str]]]:
    """
    Returns the available hosts.
    Consists of a dictionary with "agent" and "controller" keys. The
    respective lists contain a tuple with the guest name and the hostname.
    Not used in container setup.
    """
    topology = _read_topology()

    hosts = dict()

    if "guests" not in topology:
        return hosts

    for guest_name, values in topology["guests"].items():
        if values["role"] == "controller":
            hosts["controller"] = [(guest_name, values["hostname"])]
        elif values["role"] == "agent":
            if "agent" not in hosts:
                hosts["agent"] = []
            hosts["agent"].append((guest_name, values["hostname"]))

    return hosts


@pytest.fixture(scope="session")
def is_multihost_run(available_hosts: Dict[str, List[Tuple[str, str]]]) -> bool:
    """ "Returns flag if current run is a multihost test or not. Uses number of available hosts."""
    return (
        len(available_hosts) > 1
        and "controller" in available_hosts
        and "agent" in available_hosts
    )


@pytest.fixture(scope="session")
def run_with_valgrind() -> bool:
    """Returns 1 if bluechi should be run with valgrind for memory management testing"""

    return get_env_value("WITH_VALGRIND", 0) == "1"


@pytest.fixture(scope="session")
def run_with_coverage() -> bool:
    """Returns 1 if code coverage should be collected"""

    return get_env_value("WITH_COVERAGE", 0) == "1"


@pytest.fixture(scope="session")
def podman_client() -> PodmanClient:
    """Returns the podman client instance"""
    # urllib3 produces too much information in DEBUG mode, which are not interesting for bluechi tests
    urllib3_logger = logging.getLogger("urllib3.connectionpool")
    urllib3_logger.setLevel(logging.INFO)
    return PodmanClient()


@pytest.fixture(scope="session")
def bluechi_image_id(
    podman_client: PodmanClient, bluechi_image_name: str, is_multihost_run: bool
) -> Union[str, None]:
    """Returns the image ID of bluechi testing containers"""
    if is_multihost_run:
        return None

    image = next(
        iter(
            podman_client.images.list(
                filters={
                    "reference": "*{image_name}".format(image_name=bluechi_image_name)
                },
            )
        ),
        None,
    )
    assert image, f"Image '{bluechi_image_name}' was not found"
    return image.id


@pytest.fixture(scope="function")
def bluechi_ctrl_default_config(bluechi_ctrl_svc_port: str):
    return BluechiControllerConfig(file_name="ctrl.conf", port=bluechi_ctrl_svc_port)


@pytest.fixture(scope="function")
def bluechi_node_default_config(bluechi_ctrl_svc_port: str, bluechi_ctrl_host_ip: str):
    return BluechiAgentConfig(
        file_name="agent.conf",
        controller_host=bluechi_ctrl_host_ip,
        controller_port=bluechi_ctrl_svc_port,
    )


@pytest.fixture(scope="function")
def additional_ports():
    return None


@pytest.fixture(scope="function")
def bluechi_test(
    is_multihost_run: bool,
    podman_client: PodmanClient,
    available_hosts: Dict[str, List[Tuple[str, str]]],
    bluechi_image_id: str,
    bluechi_ctrl_host_port: str,
    bluechi_ctrl_svc_port: str,
    tmt_test_serial_number: str,
    tmt_test_data_dir: str,
    run_with_valgrind: bool,
    run_with_coverage: bool,
    additional_ports: dict,
    machines_ssh_user: str,
    machines_ssh_password: str,
    timeout_test_setup: int,
    timeout_test_run: int,
    timeout_collecting_test_results: int,
) -> BluechiTest:

    if is_multihost_run:
        return BluechiSSHTest(
            available_hosts,
            machines_ssh_user,
            machines_ssh_password,
            tmt_test_serial_number,
            tmt_test_data_dir,
            run_with_valgrind,
            run_with_coverage,
            timeout_test_setup,
            timeout_test_run,
            timeout_collecting_test_results,
        )

    return BluechiContainerTest(
        podman_client,
        bluechi_image_id,
        bluechi_ctrl_host_port,
        bluechi_ctrl_svc_port,
        tmt_test_serial_number,
        tmt_test_data_dir,
        run_with_valgrind,
        run_with_coverage,
        timeout_test_setup,
        timeout_test_run,
        timeout_collecting_test_results,
        additional_ports,
    )
