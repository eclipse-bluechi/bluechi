#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import datetime
import logging
import os
import re
import time
import traceback
from typing import Callable, Dict, List, Tuple

from bluechi_test.client import ContainerClient, SSHClient
from bluechi_test.command import Command
from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import (
    BluechiAgentMachine,
    BluechiControllerMachine,
    BluechiMachine,
)
from bluechi_test.util import Timeout
from podman import PodmanClient

LOGGER = logging.getLogger(__name__)


class BluechiTest:
    def __init__(
        self,
        tmt_test_serial_number: str,
        tmt_test_data_dir: str,
        run_with_valgrind: bool,
        run_with_coverage: bool,
        timeout_test_setup: int,
        timeout_test_run: int,
        timeout_collecting_test_results: int,
    ) -> None:

        self.tmt_test_serial_number = tmt_test_serial_number
        self.tmt_test_data_dir = tmt_test_data_dir
        self.run_with_valgrind = run_with_valgrind
        self.run_with_coverage = run_with_coverage
        self.timeout_test_setup = timeout_test_setup
        self.timeout_test_run = timeout_test_run
        self.timeout_collecting_test_results = timeout_collecting_test_results

        self.bluechi_controller_config: BluechiControllerConfig = None
        self.bluechi_node_configs: List[BluechiAgentConfig] = []

        self._test_init_time = datetime.datetime.now()

    def set_bluechi_controller_config(self, cfg: BluechiControllerConfig):
        self.bluechi_controller_config = cfg

    def add_bluechi_agent_config(self, cfg: BluechiAgentConfig):
        self.bluechi_node_configs.append(cfg)

    def assemble_controller_machine_name(self, cfg: BluechiControllerConfig):
        return f"{cfg.name}-{self.tmt_test_serial_number}"

    def assemble_agent_machine_name(self, cfg: BluechiAgentConfig):
        return f"{cfg.node_name}-{self.tmt_test_serial_number}"

    def setup(
        self,
    ) -> Tuple[bool, Tuple[BluechiControllerMachine, Dict[str, BluechiAgentMachine]]]:
        raise Exception("Not implemented!")

    def teardown(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        raise Exception("Not implemented!")

    def gather_logs(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        LOGGER.debug("Collecting logs from all containers...")

        if ctrl is not None:
            ctrl.gather_journal_logs(self.tmt_test_data_dir)
            if self.run_with_valgrind:
                ctrl.gather_valgrind_logs(self.tmt_test_data_dir)

        for _, node in nodes.items():
            node.gather_journal_logs(self.tmt_test_data_dir)
            if self.run_with_valgrind:
                node.gather_valgrind_logs(self.tmt_test_data_dir)

        self.gather_test_executor_logs()

    def gather_coverage(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        LOGGER.info("Collecting code coverage started")

        data_coverage_dir = f"{self.tmt_test_data_dir}/bluechi-coverage/"

        os.mkdir(f"{self.tmt_test_data_dir}/bluechi-coverage")

        if ctrl is not None:
            ctrl.gather_coverage(data_coverage_dir)

        for _, node in nodes.items():
            node.gather_coverage(data_coverage_dir)

        LOGGER.info("Collecting code coverage finished")

    def gather_test_executor_logs(self) -> None:
        LOGGER.debug("Collecting logs from test executor...")
        log_file = f"{self.tmt_test_data_dir}/journal-test_executor.log"
        try:
            logs_since = self._test_init_time.strftime("%Y-%m-%d %H:%M:%S")
            Command(f'journalctl --no-pager --since "{logs_since}" > {log_file}').run()
        except Exception as ex:
            LOGGER.error(f"Failed to gather test executor journal: {ex}")

    def shutdown_bluechi(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        LOGGER.debug("Stopping all BlueChi components in all container...")

        for _, node in nodes.items():
            node.systemctl.stop_unit("bluechi-agent")

        if ctrl is not None:
            ctrl.systemctl.stop_unit("bluechi-agent")
            ctrl.systemctl.stop_unit("bluechi-controller")

    def check_valgrind_logs(self) -> None:
        LOGGER.debug("Checking valgrind logs...")
        errors_found = False
        for filename in os.listdir(self.tmt_test_data_dir):
            if re.match(r".+-valgrind-.+\.log", filename):
                with open(os.path.join(self.tmt_test_data_dir, filename), "r") as file:
                    summary_found = False
                    for line in file.readlines():
                        if "ERROR SUMMARY" in line:
                            summary_found = True
                            errors = re.findall(r"ERROR SUMMARY: (\d+) errors", line)
                            if errors[0] != "0":
                                LOGGER.error(f"Valgrind errors found in {filename}")
                                errors_found = True
                    if not summary_found:
                        raise Exception(
                            f"Valgrind log {filename} does not contain summary, log was not finalized"
                        )
        if errors_found:
            raise Exception(
                f"Memory errors found in test. Review valgrind logs in {self.tmt_test_data_dir}"
            )

    def run(
        self,
        exec: Callable[
            [BluechiControllerMachine, Dict[str, BluechiAgentMachine]], None
        ],
    ):
        LOGGER.info("Test execution started")

        successful = False
        try:
            with Timeout(self.timeout_test_setup, "Timeout setting up BlueChi system"):
                successful, container = self.setup()
            ctrl_container, node_container = container
        except TimeoutError as ex:
            successful = False
            LOGGER.error(f"Failed to setup bluechi test: {ex}")

        if not successful:
            self.teardown(ctrl_container, node_container)
            traceback.print_exc()
            raise Exception("Failed to setup bluechi test")

        test_result = None
        try:
            with Timeout(self.timeout_test_run, "Timeout running test"):
                exec(ctrl_container, node_container)
        except Exception as ex:
            test_result = ex
            LOGGER.error(f"Failed to execute test: {ex}")
            traceback.print_exc()

        try:
            self.shutdown_bluechi(ctrl_container, node_container)
        except Exception as ex:
            test_result = ex
            LOGGER.error(f"Failed to shutdown BlueChi components: {ex}")
            traceback.print_exc()

        try:
            with Timeout(
                self.timeout_collecting_test_results,
                "Timeout collecting test artifacts",
            ):
                self.gather_logs(ctrl_container, node_container)
                if self.run_with_valgrind:
                    self.check_valgrind_logs()
                if self.run_with_coverage:
                    self.gather_coverage(ctrl_container, node_container)
        except Exception as ex:
            test_result = ex
            LOGGER.error(f"Failed to collect logs: {ex}")
            traceback.print_exc()

        self.teardown(ctrl_container, node_container)

        LOGGER.info("Test execution finished")
        if test_result is not None:
            raise test_result


class BluechiContainerTest(BluechiTest):
    def __init__(
        self,
        podman_client: PodmanClient,
        bluechi_image_id: str,
        bluechi_ctrl_host_port: str,
        bluechi_ctrl_svc_port: str,
        tmt_test_serial_number: str,
        tmt_test_data_dir: str,
        run_with_valgrind: bool,
        run_with_coverage: bool,
        timeout_test_setup: int,
        timeout_test_run: int,
        timeout_collecting_test_results: int,
        additional_ports: Dict,
    ) -> None:

        super().__init__(
            tmt_test_serial_number,
            tmt_test_data_dir,
            run_with_valgrind,
            run_with_coverage,
            timeout_test_setup,
            timeout_test_run,
            timeout_collecting_test_results,
        )

        self.podman_client = podman_client
        self.bluechi_image_id = bluechi_image_id
        self.bluechi_ctrl_host_port = bluechi_ctrl_host_port
        self.bluechi_ctrl_svc_port = bluechi_ctrl_svc_port
        self.additional_ports = additional_ports

    def setup(
        self,
    ) -> Tuple[bool, Tuple[BluechiControllerMachine, Dict[str, BluechiAgentMachine]]]:
        if self.bluechi_controller_config is None:
            raise Exception("Bluechi Controller configuration not set")

        success = True
        ctrl_container: BluechiControllerMachine = None
        node_container: Dict[str, BluechiAgentMachine] = dict()
        try:
            LOGGER.debug(
                f"Starting container for bluechi-controller with config:\
                \n{self.bluechi_controller_config.serialize()}"
            )

            name = self.assemble_controller_machine_name(self.bluechi_controller_config)
            ports = {self.bluechi_ctrl_svc_port: self.bluechi_ctrl_host_port}
            if self.additional_ports:
                ports.update(self.additional_ports)

            ctrl_container = BluechiControllerMachine(
                name,
                ContainerClient(self.podman_client, self.bluechi_image_id, name, ports),
                self.bluechi_controller_config,
            )
            if self.run_with_valgrind:
                ctrl_container.enable_valgrind()
            ctrl_container.systemctl.start_unit("bluechi-controller")
            ctrl_container.wait_for_unit_state_to_be(
                "bluechi-controller.service", "active"
            )

            for cfg in self.bluechi_node_configs:
                LOGGER.debug(
                    f"Starting container bluechi-node '{cfg.node_name}' with config:\n{cfg.serialize()}"
                )

                name = self.assemble_agent_machine_name(cfg)
                node = BluechiAgentMachine(
                    name,
                    ContainerClient(
                        self.podman_client, self.bluechi_image_id, name, {}
                    ),
                    cfg,
                )
                node_container[cfg.node_name] = node

                if self.run_with_valgrind:
                    node.enable_valgrind()

                node.systemctl.start_unit("bluechi-agent")
                node.wait_for_unit_state_to_be("bluechi-agent.service", "active")

        except Exception as ex:
            success = False
            LOGGER.error(f"Failed to setup bluechi container: {ex}")
            traceback.print_exc()

        if self.run_with_valgrind:
            # Give some more time for bluechi to start and connect while running with valgrind
            time.sleep(2)

        return (success, (ctrl_container, node_container))

    def teardown(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        LOGGER.debug("Stopping and removing all container...")

        if ctrl is not None and isinstance(ctrl.client, ContainerClient):
            if ctrl.client.container.status == "running":
                kw_params = {"timeout": 0}
                ctrl.client.container.stop(**kw_params)
            ctrl.client.container.remove()

        for _, node in nodes.items():
            if node is not None and isinstance(node.client, ContainerClient):
                if node.client.container.status == "running":
                    kw_params = {"timeout": 0}
                    node.client.container.stop(**kw_params)
                node.client.container.remove()


class BluechiSSHTest(BluechiTest):
    def __init__(
        self,
        available_hosts: Dict[str, List[Tuple[str, str]]],
        ssh_user: str,
        ssh_password: str,
        tmt_test_serial_number: str,
        tmt_test_data_dir: str,
        run_with_valgrind: bool,
        run_with_coverage: bool,
        timeout_test_setup: int,
        timeout_test_run: int,
        timeout_collecting_test_results: int,
    ) -> None:

        super().__init__(
            tmt_test_serial_number,
            tmt_test_data_dir,
            run_with_valgrind,
            run_with_coverage,
            timeout_test_setup,
            timeout_test_run,
            timeout_collecting_test_results,
        )

        self.available_hosts = available_hosts
        self.ssh_user = ssh_user
        self.ssh_password = ssh_password

    def add_bluechi_agent_config(self, cfg: BluechiAgentConfig):
        if len(self.bluechi_node_configs) >= len(self.available_hosts["agent"]):
            raise Exception(
                f"Failed to add agent config: "
                f"Maximum requested hosts reached (# available: {len(self.available_hosts['agent'])})"
            )
        super().add_bluechi_agent_config(cfg)

    def setup(
        self,
    ) -> Tuple[bool, Tuple[BluechiControllerMachine, Dict[str, BluechiAgentMachine]]]:
        if self.bluechi_controller_config is None:
            raise Exception("Bluechi Controller configuration not set")
        if len(self.available_hosts) < 1:
            raise Exception("No available hosts!")

        success = True
        ctrl_machine: BluechiControllerMachine = None
        agent_machines: Dict[str, BluechiAgentMachine] = dict()

        try:
            LOGGER.debug(
                f"Starting bluechi-controller with config:\
                \n{self.bluechi_controller_config.serialize()}"
            )

            name = self.assemble_controller_machine_name(self.bluechi_controller_config)
            guest, host = self.available_hosts["controller"][0]

            LOGGER.debug(
                f"Setting up controller machine on guest '{guest}' with host '{host}'"
            )
            ctrl_machine = BluechiControllerMachine(
                name,
                SSHClient(host, user=self.ssh_user, password=self.ssh_password),
                self.bluechi_controller_config,
            )
            if self.run_with_valgrind:
                ctrl_machine.enable_valgrind()
            ctrl_machine.systemctl.start_unit("bluechi-controller.service")
            ctrl_machine.wait_for_unit_state_to_be(
                "bluechi-controller.service", "active"
            )

            i = 0
            for cfg in self.bluechi_node_configs:
                LOGGER.debug(
                    f"Starting bluechi-agent '{cfg.node_name}' with config:\n{cfg.serialize()}"
                )

                name = self.assemble_agent_machine_name(cfg)
                guest, host = self.available_hosts["agent"][i]
                i = i + 1

                LOGGER.debug(
                    f"Setting up agent machine on guest '{guest}' with host '{host}'"
                )
                agent_machine = BluechiAgentMachine(
                    name,
                    SSHClient(host, user=self.ssh_user, password=self.ssh_password),
                    cfg,
                )

                agent_machines[cfg.node_name] = agent_machine

                if self.run_with_valgrind:
                    agent_machine.enable_valgrind()

                agent_machine.systemctl.start_unit("bluechi-agent.service")
                agent_machine.wait_for_unit_state_to_be(
                    "bluechi-agent.service", "active"
                )

        except Exception as ex:
            success = False
            LOGGER.error(f"Failed to setup bluechi machines: {ex}")
            traceback.print_exc()

        if self.run_with_valgrind:
            # Give some more time for bluechi to start and connect while running with valgrind
            time.sleep(2)

        return (success, (ctrl_machine, agent_machines))

    def teardown(
        self, ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]
    ):
        if ctrl is not None:
            self._cleanup(ctrl)

        for _, node in nodes.items():
            bluechictl_started_services: List[str] = []
            if node.config.node_name in ctrl.bluechictl.tracked_services:
                bluechictl_started_services = ctrl.bluechictl.tracked_services[
                    node.config.node_name
                ]
            self._cleanup(node, bluechictl_started_services)

    def _cleanup(self, machine: BluechiMachine, other_tracked_service: List[str] = []):

        LOGGER.info("Stopping and resetting tracked units...")
        tracked_services = machine.systemctl.tracked_services + other_tracked_service
        for service in tracked_services:
            machine.systemctl.stop_unit(service, check_result=False)
            machine.systemctl.reset_failed_for_unit(service, check_result=False)

        LOGGER.info("Restoring changed files...")
        for changed_file in machine.changed_files:
            backup_file = changed_file + BluechiMachine.backup_file_suffix
            machine.client.exec_run(f"mv {backup_file} {changed_file}")

        LOGGER.info("Removing created files...")
        cmd_rm_created_files = "rm " + " ".join(machine.created_files)
        machine.client.exec_run(cmd_rm_created_files)

        # ensure changed systemd services are reloaded
        machine.systemctl.daemon_reload()

        # ensure that both, agent and controller can be started again
        # if they failed to quickly previously
        machine.systemctl.reset_failed_for_unit("bluechi-agent", check_result=False)
        machine.systemctl.reset_failed_for_unit(
            "bluechi-controller", check_result=False
        )

        machine.cleanup_valgrind_logs()
        machine.cleanup_journal_logs()
        machine.cleanup_coverage()
