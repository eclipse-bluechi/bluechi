#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import List


class BluechiConfig:

    # use a smaller max line length than possible
    # to prevent any accidental faults
    MAX_LINE_LENGTH = 400

    def __init__(self, file_name: str) -> None:
        self.file_name = file_name

    def serialize(self) -> str:
        raise Exception("Not implemented")

    def get_confd_dir(self) -> str:
        raise Exception("Not implemented")


class BluechiControllerPerNodeConfig:

    def __init__(
        self,
        node_name: str,
        allowed: bool = True,
        required_selinux_context: str = "",
        proxy_to: List[str] = [],
    ):
        self.node_name = node_name
        self.allowed = allowed
        self.required_selinux_context = required_selinux_context
        self.proxy_to = proxy_to

    def serialize(self) -> str:
        proxy_to = ",\n ".join(self.proxy_to)

        return f"""[node {self.node_name}]
Allowed={self.allowed}
RequiredSelinuxContext={self.required_selinux_context}
AllowDependenciesOn={proxy_to}
"""


class BluechiControllerConfig(BluechiConfig):

    confd_dir = "/etc/bluechi/controller.conf.d"

    def __init__(
        self,
        file_name: str,
        name: str = "bluechi-controller",
        port: str = "8420",
        allowed_node_names: List[str] = [],
        heartbeat_interval: str = "0",
        node_heartbeat_threshold: str = "6000",
        log_level: str = "DEBUG",
        log_target: str = "journald",
        log_is_quiet: bool = False,
        use_tcp: bool = True,
        use_uds: bool = True,
        per_node_config: List[BluechiControllerPerNodeConfig] = [],
    ) -> None:
        super().__init__(file_name)

        self.name = name
        self.port = port
        self.allowed_node_names = allowed_node_names
        self.heartbeat_interval = heartbeat_interval
        self.node_heartbeat_threshold = node_heartbeat_threshold
        self.log_level = log_level
        self.log_target = log_target
        self.log_is_quiet = log_is_quiet
        self.use_tcp = use_tcp
        self.use_uds = use_uds
        self.per_node_config = per_node_config

    def serialize(self) -> str:
        # use one line for each node name so the max line length
        # supported by bluechi is never exceeded
        allowed_node_names = ",\n ".join(self.allowed_node_names)
        per_node_configs = "\n".join(
            per_node_config.serialize() for per_node_config in self.per_node_config
        )

        return f"""[bluechi-controller]
ControllerPort={self.port}
AllowedNodeNames={allowed_node_names}
HeartbeatInterval={self.heartbeat_interval}
NodeHeartbeatThreshold={self.node_heartbeat_threshold}
LogLevel={self.log_level}
LogTarget={self.log_target}
LogIsQuiet={self.log_is_quiet}
UseTCP={self.use_tcp}
UseUDS={self.use_uds}
{per_node_configs}
"""

    def get_confd_dir(self) -> str:
        return self.confd_dir

    def deep_copy(self) -> "BluechiControllerConfig":
        cfg = BluechiControllerConfig(self.file_name)
        cfg.name = self.name
        cfg.port = self.port
        cfg.allowed_node_names = self.allowed_node_names
        cfg.heartbeat_interval = self.heartbeat_interval
        cfg.node_heartbeat_threshold = self.node_heartbeat_threshold
        cfg.log_level = self.log_level
        cfg.log_target = self.log_target
        cfg.log_is_quiet = self.log_is_quiet
        cfg.use_tcp = self.use_tcp
        cfg.use_uds = self.use_uds
        return cfg


class BluechiAgentConfig(BluechiConfig):

    confd_dir = "/etc/bluechi/agent.conf.d"

    def __init__(
        self,
        file_name: str,
        node_name: str = "",
        controller_host: str = "",
        controller_port: str = "8420",
        controller_address: str = "",
        heartbeat_interval: str = "2000",
        controller_heartbeat_threshold: str = "0",
        log_level: str = "DEBUG",
        log_target: str = "journald",
        log_is_quiet: bool = False,
    ) -> None:
        super().__init__(file_name)

        self.node_name = node_name
        self.controller_host = controller_host
        self.controller_port = controller_port
        self.controller_address = controller_address
        self.heartbeat_interval = heartbeat_interval
        self.controller_heartbeat_threshold = controller_heartbeat_threshold
        self.log_level = log_level
        self.log_target = log_target
        self.log_is_quiet = log_is_quiet

    def serialize(self) -> str:
        return f"""[bluechi-agent]
NodeName={self.node_name}
ControllerHost={self.controller_host}
ControllerPort={self.controller_port}
ControllerAddress={self.controller_address}
HeartbeatInterval={self.heartbeat_interval}
ControllerHeartbeatThreshold={self.controller_heartbeat_threshold}
LogLevel={self.log_level}
LogTarget={self.log_target}
LogIsQuiet={self.log_is_quiet}
"""

    def get_confd_dir(self) -> str:
        return self.confd_dir

    def deep_copy(self) -> "BluechiAgentConfig":
        cfg = BluechiAgentConfig(self.file_name)
        cfg.node_name = self.node_name
        cfg.controller_host = self.controller_host
        cfg.controller_port = self.controller_port
        cfg.controller_address = self.controller_address
        cfg.heartbeat_interval = self.heartbeat_interval
        cfg.controller_heartbeat_threshold = self.controller_heartbeat_threshold
        cfg.log_level = self.log_level
        cfg.log_target = self.log_target
        cfg.log_is_quiet = self.log_is_quiet
        return cfg
