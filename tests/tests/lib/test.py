# SPDX-License-Identifier: GPL-2.0-or-later

from podman import PodmanClient
from typing import List, Dict, Callable, Tuple

from tests.lib.config import HirteControllerConfig, HirteNodeConfig
from tests.lib.container import HirteContainer


class HirteTest():

    def __init__(
            self,
            podman_client: PodmanClient,
            hirte_image_id: str,
            hirte_network_name: str,
            hirte_ctrl_host_port: str,
            hirte_ctrl_svc_port: str,
            tmt_test_data_dir: str) -> None:

        self.podman_client = podman_client
        self.hirte_image_id = hirte_image_id
        self.hirte_network_name = hirte_network_name
        self.hirte_ctrl_host_port = hirte_ctrl_host_port
        self.hirte_ctrl_svc_port = hirte_ctrl_svc_port
        self.tmt_test_data_dir = tmt_test_data_dir

        self.hirte_controller_config: HirteControllerConfig = None
        self.hirte_node_configs: List[HirteNodeConfig] = []

    def set_hirte_controller_config(self, cfg: HirteControllerConfig):
        self.hirte_controller_config = cfg

    def add_hirte_node_config(self, cfg: HirteNodeConfig):
        self.hirte_node_configs.append(cfg)

    def setup(self) -> Tuple[bool, Tuple[HirteContainer, Dict[str, HirteContainer]]]:
        if self.hirte_controller_config is None:
            raise Exception("Hirte Controller configuration not set")

        success = True
        ctrl_container: HirteContainer = None
        node_container: Dict[str, HirteContainer] = dict()
        try:
            print(f"Starting container for hirte-controller with config:\n{self.hirte_controller_config.serialize()}")

            c = self.podman_client.containers.run(
                name=self.hirte_controller_config.name,
                image=self.hirte_image_id,
                detach=True,
                ports={self.hirte_ctrl_svc_port: self.hirte_ctrl_host_port},
                networks={self.hirte_network_name: {}},
            )

            ctrl_container = HirteContainer(c, self.hirte_controller_config)
            ctrl_container.exec_run('systemctl start hirte')

            for cfg in self.hirte_node_configs:
                print(f"Starting container hirte-node '{cfg.node_name}' with config:\n{cfg.serialize()}")

                c = self.podman_client.containers.run(
                    name=cfg.node_name,
                    image=self.hirte_image_id,
                    detach=True,
                    networks={self.hirte_network_name: {}},
                )
                node = HirteContainer(c, cfg)
                node.exec_run('systemctl start hirte-agent')

                node_container[cfg.node_name] = node
        except Exception as ex:
            success = False
            print(f"Failed to setup hirte container: {ex}")

        return (success, (ctrl_container, node_container))

    def gather_logs(self, ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
        print("Collecting logs from all container...")

        if ctrl is not None:
            ctrl.gather_journal_logs(self.tmt_test_data_dir)

        for _, node in nodes.items():
            node.gather_journal_logs(self.tmt_test_data_dir)

    def teardown(self, ctrl: HirteContainer, nodes: Dict[str, HirteContainer]):
        print("Stopping and removing all container...")

        if ctrl is not None:
            ctrl.cleanup()

        for _, node in nodes.items():
            node.cleanup()

    def run(self, exec: Callable[[HirteContainer, Dict[str, HirteContainer]], None]):
        successful, container = self.setup()
        ctrl_container, node_container = container

        if not successful:
            self.teardown(ctrl_container, node_container)
            raise Exception("Failed to setup hirte test")

        try:
            exec(ctrl_container, node_container)
        finally:
            self.gather_logs(ctrl_container, node_container)
            self.teardown(ctrl_container, node_container)
