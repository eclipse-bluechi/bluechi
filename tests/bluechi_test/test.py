# SPDX-License-Identifier: LGPL-2.1-or-later

import traceback

from podman import PodmanClient
from typing import List, Dict, Callable, Tuple

from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig
from bluechi_test.container import BluechiNodeContainer, BluechiControllerContainer


class BluechiTest():

    def __init__(
            self,
            podman_client: PodmanClient,
            bluechi_image_id: str,
            bluechi_network_name: str,
            bluechi_ctrl_host_port: str,
            bluechi_ctrl_svc_port: str,
            tmt_test_data_dir: str) -> None:

        self.podman_client = podman_client
        self.bluechi_image_id = bluechi_image_id
        self.bluechi_network_name = bluechi_network_name
        self.bluechi_ctrl_host_port = bluechi_ctrl_host_port
        self.bluechi_ctrl_svc_port = bluechi_ctrl_svc_port
        self.tmt_test_data_dir = tmt_test_data_dir

        self.bluechi_controller_config: BluechiControllerConfig = None
        self.bluechi_node_configs: List[BluechiNodeConfig] = []

    def set_bluechi_controller_config(self, cfg: BluechiControllerConfig):
        self.bluechi_controller_config = cfg

    def add_bluechi_node_config(self, cfg: BluechiNodeConfig):
        self.bluechi_node_configs.append(cfg)

    def setup(self) -> Tuple[bool, Tuple[BluechiControllerContainer, Dict[str, BluechiNodeContainer]]]:
        if self.bluechi_controller_config is None:
            raise Exception("Bluechi Controller configuration not set")

        success = True
        ctrl_container: BluechiControllerContainer = None
        node_container: Dict[str, BluechiNodeContainer] = dict()
        try:
            print(
                f"Starting container for bluechi-controller with config:\n{self.bluechi_controller_config.serialize()}")

            c = self.podman_client.containers.run(
                name=self.bluechi_controller_config.name,
                image=self.bluechi_image_id,
                detach=True,
                ports={self.bluechi_ctrl_svc_port: self.bluechi_ctrl_host_port},
                networks={self.bluechi_network_name: {}},
            )
            c.wait(condition="running")

            ctrl_container = BluechiControllerContainer(c, self.bluechi_controller_config)
            ctrl_container.exec_run('systemctl start bluechi')

            for cfg in self.bluechi_node_configs:
                print(f"Starting container bluechi-node '{cfg.node_name}' with config:\n{cfg.serialize()}")

                c = self.podman_client.containers.run(
                    name=cfg.node_name,
                    image=self.bluechi_image_id,
                    detach=True,
                    networks={self.bluechi_network_name: {}},
                )
                c.wait(condition="running")

                node = BluechiNodeContainer(c, cfg)
                node_container[cfg.node_name] = node

                node.exec_run('systemctl start bluechi-agent')

        except Exception as ex:
            success = False
            print(f"Failed to setup bluechi container: {ex}")

        return (success, (ctrl_container, node_container))

    def gather_logs(self, ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
        print("Collecting logs from all container...")

        if ctrl is not None:
            ctrl.gather_journal_logs(self.tmt_test_data_dir)

        for _, node in nodes.items():
            node.gather_journal_logs(self.tmt_test_data_dir)

    def teardown(self, ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
        print("Stopping and removing all container...")

        if ctrl is not None:
            ctrl.cleanup()

        for _, node in nodes.items():
            node.cleanup()

    def run(self, exec: Callable[[BluechiControllerContainer, Dict[str, BluechiNodeContainer]], None]):
        successful, container = self.setup()
        ctrl_container, node_container = container

        if not successful:
            self.teardown(ctrl_container, node_container)
            traceback.print_exc()
            raise Exception("Failed to setup bluechi test")

        try:
            exec(ctrl_container, node_container)
        finally:
            self.gather_logs(ctrl_container, node_container)
            self.teardown(ctrl_container, node_container)
