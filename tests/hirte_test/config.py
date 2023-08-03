# SPDX-License-Identifier: GPL-2.0-or-later

from typing import List


class HirteConfig():

    # use a smaller max line length than possible
    # to prevent any accidental faults
    MAX_LINE_LENGTH = 400

    def __init__(self, file_name: str) -> None:
        self.file_name = file_name

    def serialize(self) -> str:
        raise Exception("Not implemented")

    def get_confd_dir(self) -> str:
        raise Exception("Not implemented")


class HirteControllerConfig(HirteConfig):

    confd_dir = "/etc/hirte/hirte.conf.d"

    def __init__(
            self,
            file_name: str,
            name: str = "hirte-controller",
            port: str = "8420",
            allowed_node_names: List[str] = [],
            log_level: str = "DEBUG",
            log_target: str = "journald",
            log_is_quiet: bool = False) -> None:
        super().__init__(file_name)

        self.name = name
        self.port = port
        self.allowed_node_names = allowed_node_names
        self.log_level = log_level
        self.log_target = log_target
        self.log_is_quiet = log_is_quiet

    def serialize(self) -> str:
        # use one line for each node name so the max line length
        # supported by hirte is never exceeded
        allowed_node_names = ",\n ".join(self.allowed_node_names)

        return f"""[hirte]
ManagerPort={self.port}
AllowedNodeNames={allowed_node_names}
LogLevel={self.log_level}
LogTarget={self.log_target}
LogIsQuiet={self.log_is_quiet}
"""

    def get_confd_dir(self) -> str:
        return self.confd_dir

    def deep_copy(self) -> "HirteControllerConfig":
        cfg = HirteControllerConfig(self.file_name)
        cfg.name = self.name
        cfg.port = self.port
        cfg.allowed_node_names = self.allowed_node_names
        cfg.log_level = self.log_level
        cfg.log_target = self.log_target
        cfg.log_is_quiet = self.log_is_quiet
        return cfg


class HirteNodeConfig(HirteConfig):

    confd_dir = "/etc/hirte/agent.conf.d"

    def __init__(
            self,
            file_name: str,
            node_name: str = "",
            manager_host: str = "",
            manager_port: str = "8420",
            manager_address: str = "",
            heartbeat_interval: str = "2000",
            log_level: str = "DEBUG",
            log_target: str = "journald",
            log_is_quiet: bool = False) -> None:
        super().__init__(file_name)

        self.node_name = node_name
        self.manager_host = manager_host
        self.manager_port = manager_port
        self.manager_address = manager_address
        self.heartbeat_interval = heartbeat_interval
        self.log_level = log_level
        self.log_target = log_target
        self.log_is_quiet = log_is_quiet

    def serialize(self) -> str:
        return f"""[hirte-agent]
NodeName={self.node_name}
ManagerHost={self.manager_host}
ManagerPort={self.manager_port}
ManagerAddress={self.manager_address}
HeartbeatInterval={self.heartbeat_interval}
LogLevel={self.log_level}
LogTarget={self.log_target}
LogIsQuiet={self.log_is_quiet}
"""

    def get_confd_dir(self) -> str:
        return self.confd_dir

    def deep_copy(self) -> "HirteNodeConfig":
        cfg = HirteNodeConfig(self.file_name)
        cfg.node_name = self.node_name
        cfg.manager_host = self.manager_host
        cfg.manager_port = self.manager_port
        cfg.manager_address = self.manager_address
        cfg.heartbeat_interval = self.heartbeat_interval
        cfg.log_level = self.log_level
        cfg.log_target = self.log_target
        cfg.log_is_quiet = self.log_is_quiet
        return cfg
