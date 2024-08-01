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

    def serialize(self) -> str:
        # use one line for each node name so the max line length
        # supported by bluechi is never exceeded
        allowed_node_names = ",\n ".join(self.allowed_node_names)

        return f"""[bluechi-controller]
ControllerPort={self.port}
AllowedNodeNames={allowed_node_names}
HeartbeatInterval={self.heartbeat_interval}
NodeHeartbeatThreshold={self.node_heartbeat_threshold}
LogLevel={self.log_level}
LogTarget={self.log_target}
LogIsQuiet={self.log_is_quiet}
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
