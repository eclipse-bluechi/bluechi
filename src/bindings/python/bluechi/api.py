# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Generated file. DO NOT EDIT!
#

from __future__ import annotations

from typing import Callable, Tuple, Dict, List
from dasbus.typing import (
    Bool,
    Double,
    Str,
    Int,
    Byte,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    ObjPath,
    Structure,
)

# File has been renamed to UnixFD in following PR included in v1.7
# https://github.com/rhinstaller/dasbus/pull/70
try:
    from dasbus.typing import File
except ImportError:
    from dasbus.typing import UnixFD

from dasbus.client.proxy import InterfaceProxy, ObjectProxy
from dasbus.connection import MessageBus, SystemMessageBus, SessionMessageBus

import gi

gi.require_version("GLib", "2.0")
from gi.repository.GLib import Variant  # noqa: E402


BC_DEFAULT_PORT = 842
BC_DEFAULT_HOST = "127.0.0.1"

BC_DBUS_INTERFACE = "org.eclipse.bluechi"
BC_OBJECT_PATH = "/org/eclipse/bluechi"
BC_AGENT_DBUS_INTERFACE = "org.eclipse.bluechi.Agent"


class ApiBase:
    def __init__(self, bus: MessageBus = None, use_systembus=True) -> None:
        self.use_systembus = use_systembus

        if bus is not None:
            self.bus = bus
        elif self.use_systembus:
            self.bus = SystemMessageBus()
        else:
            self.bus = SessionMessageBus()

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        raise Exception("Not implemented!")


class Monitor(ApiBase):
    def __init__(
        self, monitor_path: ObjPath, bus: MessageBus = None, use_systembus=True
    ) -> None:
        super().__init__(bus, use_systembus)

        self.monitor_path = monitor_path
        self.monitor_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.monitor_proxy is None:
            self.monitor_proxy = self.bus.get_proxy(
                BC_DBUS_INTERFACE, self.monitor_path
            )

        return self.monitor_proxy

    def close(self) -> None:
        self.get_proxy().Close()

    def subscribe(self, node: str, unit: str) -> UInt32:
        return self.get_proxy().Subscribe(
            node,
            unit,
        )

    def unsubscribe(self, id: UInt32) -> None:
        self.get_proxy().Unsubscribe(
            id,
        )

    def subscribe_list(self, targets: Tuple[str, List[str]]) -> UInt32:
        return self.get_proxy().SubscribeList(
            targets,
        )

    def on_unit_properties_changed(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
                Structure,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().UnitPropertiesChanged.connect(callback)

    def on_unit_state_changed(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
                str,
                str,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().UnitStateChanged.connect(callback)

    def on_unit_new(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().UnitNew.connect(callback)

    def on_unit_removed(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().UnitRemoved.connect(callback)


class Metrics(ApiBase):
    def __init__(
        self, metrics_path: ObjPath, bus: MessageBus = None, use_systembus=True
    ) -> None:
        super().__init__(bus, use_systembus)

        self.metrics_path = metrics_path
        self.metrics_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.metrics_proxy is None:
            self.metrics_proxy = self.bus.get_proxy(
                BC_DBUS_INTERFACE, self.metrics_path
            )

        return self.metrics_proxy

    def on_start_unit_job_metrics(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
                UInt64,
                UInt64,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().StartUnitJobMetrics.connect(callback)

    def on_agent_job_metrics(
        self,
        callback: Callable[
            [
                str,
                str,
                str,
                UInt64,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().AgentJobMetrics.connect(callback)


class Job(ApiBase):
    def __init__(
        self, job_path: ObjPath, bus: MessageBus = None, use_systembus=True
    ) -> None:
        super().__init__(bus, use_systembus)

        self.job_path = job_path
        self.job_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.job_proxy is None:
            self.job_proxy = self.bus.get_proxy(BC_DBUS_INTERFACE, self.job_path)

        return self.job_proxy

    def cancel(self) -> None:
        self.get_proxy().Cancel()

    @property
    def id(self) -> UInt32:
        return self.get_proxy().Id

    @property
    def node(self) -> str:
        return self.get_proxy().Node

    @property
    def unit(self) -> str:
        return self.get_proxy().Unit

    @property
    def job_type(self) -> str:
        return self.get_proxy().JobType

    @property
    def state(self) -> str:
        return self.get_proxy().State


class Manager(ApiBase):
    def __init__(self, bus: MessageBus = None, use_systembus=True) -> None:
        super().__init__(bus, use_systembus)

        self.manager_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.manager_proxy is None:
            self.manager_proxy = self.bus.get_proxy(BC_DBUS_INTERFACE, BC_OBJECT_PATH)

        return self.manager_proxy

    def list_units(
        self,
    ) -> List[Tuple[str, str, str, str, str, str, ObjPath, UInt32, str, ObjPath]]:
        return self.get_proxy().ListUnits()

    def list_nodes(self) -> List[Tuple[str, ObjPath, str]]:
        return self.get_proxy().ListNodes()

    def get_node(self, name: str) -> ObjPath:
        return self.get_proxy().GetNode(
            name,
        )

    def create_monitor(self) -> ObjPath:
        return self.get_proxy().CreateMonitor()

    def enable_metrics(self) -> None:
        self.get_proxy().EnableMetrics()

    def disable_metrics(self) -> None:
        self.get_proxy().DisableMetrics()

    def set_log_level(self, loglevel: str) -> None:
        self.get_proxy().SetLogLevel(
            loglevel,
        )

    def on_job_new(
        self,
        callback: Callable[
            [
                UInt32,
                ObjPath,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().JobNew.connect(callback)

    def on_job_removed(
        self,
        callback: Callable[
            [
                UInt32,
                ObjPath,
                str,
                str,
                str,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().JobRemoved.connect(callback)

    def on_node_connection_state_changed(
        self,
        callback: Callable[
            [
                str,
                str,
            ],
            None,
        ],
    ) -> None:
        """
        callback:
        """
        self.get_proxy().NodeConnectionStateChanged.connect(callback)


class Node(ApiBase):
    def __init__(
        self, node_name: str, bus: MessageBus = None, use_systembus=True
    ) -> None:
        super().__init__(bus, use_systembus)

        self.node_name = node_name
        self.node_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.node_proxy is None:
            manager = self.bus.get_proxy(BC_DBUS_INTERFACE, BC_OBJECT_PATH)

            node_path = manager.GetNode(self.node_name)
            self.node_proxy = self.bus.get_proxy(BC_DBUS_INTERFACE, node_path)

        return self.node_proxy

    def start_unit(self, name: str, mode: str) -> ObjPath:
        return self.get_proxy().StartUnit(
            name,
            mode,
        )

    def stop_unit(self, name: str, mode: str) -> ObjPath:
        return self.get_proxy().StopUnit(
            name,
            mode,
        )

    def reload_unit(self, name: str, mode: str) -> ObjPath:
        return self.get_proxy().ReloadUnit(
            name,
            mode,
        )

    def restart_unit(self, name: str, mode: str) -> ObjPath:
        return self.get_proxy().RestartUnit(
            name,
            mode,
        )

    def get_unit_properties(self, name: str, interface: str) -> Structure:
        return self.get_proxy().GetUnitProperties(
            name,
            interface,
        )

    def get_unit_property(self, name: str, interface: str, property: str) -> Variant:
        return self.get_proxy().GetUnitProperty(
            name,
            interface,
            property,
        )

    def set_unit_properties(
        self, name: str, runtime: bool, keyvalues: List[Tuple[str, Variant]]
    ) -> None:
        self.get_proxy().SetUnitProperties(
            name,
            runtime,
            keyvalues,
        )

    def enable_unit_files(
        self, files: List[str], runtime: bool, force: bool
    ) -> Tuple[bool, List[Tuple[str, str, str]],]:
        return self.get_proxy().EnableUnitFiles(
            files,
            runtime,
            force,
        )

    def disable_unit_files(
        self, files: List[str], runtime: bool
    ) -> List[Tuple[str, str, str]]:
        return self.get_proxy().DisableUnitFiles(
            files,
            runtime,
        )

    def list_units(
        self,
    ) -> List[Tuple[str, str, str, str, str, str, ObjPath, UInt32, str, ObjPath]]:
        return self.get_proxy().ListUnits()

    def reload(self) -> None:
        self.get_proxy().Reload()

    def set_log_level(self, level: str) -> None:
        self.get_proxy().SetLogLevel(
            level,
        )

    @property
    def name(self) -> str:
        return self.get_proxy().Name

    @property
    def status(self) -> str:
        return self.get_proxy().Status

    @property
    def last_seen_timestamp(self) -> UInt64:
        return self.get_proxy().LastSeenTimestamp


class Agent(ApiBase):
    def __init__(self, bus: MessageBus = None, use_systembus=True) -> None:
        super().__init__(bus, use_systembus)

        self.agent_proxy = None

    def get_proxy(self) -> InterfaceProxy | ObjectProxy:
        if self.agent_proxy is None:
            self.agent_proxy = self.bus.get_proxy(
                BC_AGENT_DBUS_INTERFACE, BC_OBJECT_PATH
            )

        return self.agent_proxy

    def create_proxy(self, local_service_name: str, node: str, unit: str) -> None:
        self.get_proxy().CreateProxy(
            local_service_name,
            node,
            unit,
        )

    def remove_proxy(self, local_service_name: str, node: str, unit: str) -> None:
        self.get_proxy().RemoveProxy(
            local_service_name,
            node,
            unit,
        )
