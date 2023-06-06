# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# vim:sw=4:ts=4:et
import dasbus.connection as dbus
from .config import HIRTE_DBUS_INTERFACE, HIRTE_OBJECT_PATH
from collections import namedtuple
from dasbus.typing import Variant
from dasbus.loop import EventLoop
from datetime import datetime


class Hirte:
    """ Hirte object implementation """

    def __init__(
         self
    ) -> None:
        """
        Init the class
        """
        super().__init__()
        self.bus = dbus.SystemMessageBus()
        self.manager = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            HIRTE_OBJECT_PATH
        )

    def Stop(
        self,
        nodeName,
        unitName
    ):
        """
        Stop() is a method from Hirte class which will STOP a systemd unit
        remotely using nodeName as reference for the node.

        Uses the method _action() to execute the operation.

        Args:
            nodeName:
                The name of node to stop the unit

            unitName:
                Unit Name to be stopped, i.e: chronyd.service

        Returns:
        """
        self._action(
            "stop",
            nodeName,
            unitName
        )

    def Enable(
        self,
        nodeName,
        unitName
    ):
        """
        Enable() is a method from Hirte() which will ENABLE a systemd unit(s)
        remotely using nodeName as reference for the node.

        Uses the method _action() to execute the operation.

        Args:
            nodeName:
                The name of node to enable the unit

            unitName:
                Unit Name to be enabled, i.e: chronyd.service

        Returns:
        """
        self._action(
            "enable",
            nodeName,
            unitName
        )

    def Disable(
        self,
        nodeName,
        unitName
    ):
        """
        Disable() is a method from Hirte() which will DISABLE a systemd unit(s)
        remotely using nodeName as reference for the node.

        Uses the method _action() to execute the operation.

        Args:
            nodeName:
                The name of node to disable the unit

            unitName:
                Unit Name to be disabled, i.e: chronyd.service

        Returns:
        """
        self._action(
            "disable",
            nodeName,
            unitName
        )

    def Start(
        self,
        nodeName,
        unitName
    ):
        """
        Start() is a method from Hirte class which will START a systemd unit
        remotely using nodeName as reference for the node.

        Uses the method _action() to execute the operation.

        Args:
            nodeName:
                The name of node to start the unit

            unitName:
                Unit Name to be started, i.e: chronyd.service

        Returns:
        """
        self._action(
            "start",
            nodeName,
            unitName
        )

    def _action(
        self,
        operation,
        nodeName,
        unitName,
    ):
        """
        The private method _action() will encapsulate the public methods such:
        Enable(), Disable(), Start(), Stop()

        Example: The public method Enable() will be calling _enableUnit().

        Args:
            operation:
                Systemd operation to be executed:
                    start, stop, enable and disable

            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name to be started, i.e: chronyd.service

        Returns:
        """

        if operation == "disable":
            return self._disableUnit(
                nodeName,
                unitName
            )
        elif operation == "enable":
            return self._enableUnit(
                nodeName,
                unitName
            )
        elif operation == "start" or operation == "stop":
            return self._executeStartStop(
                operation,
                nodeName,
                unitName
            )

    def _executeStartStop(
        self,
        operation,
        nodeName,
        unitName
    ):
        """
        This private method will implement the execution of
        Start() and Stop() in a remote systemd.

        Args:
            operation:
                Systemd operation to be executed:
                    start or stop

            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name to be started or stopped, i.e: chronyd.service

        Returns:
        """
        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        loop = EventLoop()

        def job_removed(
            id,
            job_path,
            node_name,
            unit,
            result
        ):
            if job_path == res_path:
                run_time = (datetime.utcnow() - operation_time).total_seconds()
                run_time = "{:.1f}".format(run_time * 1000)
                print(f"{operation}ed {unit} on node {node_name}"
                      f" with result {result} in {run_time} msec")
                loop.quit()

        operation_time = datetime.utcnow()

        # JobRemoved is emitted each time a new job is dequeued
        # or the underlying systemd job finished.
        self.manager.JobRemoved.connect(job_removed)
        if operation == "start":
            res_path = node.StartUnit(unitName, "replace")
        elif operation == "stop":
            res_path = node.StopUnit(unitName, "replace")

        loop.run()

    def _disableUnit(
        self,
        nodeName,
        unitName
    ):
        """
        _disableUnit() implements the Disable operation
        Args:
            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name to be disabled

        Returns:
        """
        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        node.DisableUnitFiles(
            unitName,
            False,
        )

    def _enableUnit(
        self,
        nodeName,
        unitName
    ):
        """
        _enableUnit() implements the Enable operation
        Args:
            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name to be enabled

        Returns:
        """

        EnabledServiceInfo = namedtuple(
            "EnabledServicesInfo",
            ["op_type", "symlink_file", "symlink_dest"]
        )

        EnableResponse = namedtuple(
            "EnableResponse",
            ["carries_install_info", "enabled_services_info"]
        )

        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        response = node.EnableUnitFiles(
            unitName,
            False,
            False
        )

        enable_response = EnableResponse(*response)
        if enable_response.carries_install_info:
            print("The unit files included enablement information")
        else:
            print("The unit files did not include any enablement information")

        for e in enable_response.enabled_services_info:
            enabled_service_info = EnabledServiceInfo(*e)
            if enabled_service_info.op_type == "symlink":
                print(f"Created symlink {enabled_service_info.symlink_file}"
                      " -> {enabled_service_info.symlink_dest}")
            elif enabled_service_info.op_type == "unlink":
                print(f"Removed \"{enabled_service_info.symlink_file}\".")

    def ListAllNodes(
        self,
    ):
        """
        The public method ListAllNodes() will print all Hirte nodes.

        Returns:
        """
        NodeInfo = namedtuple(
            "NodeInfo", [
                "name",
                "object_path",
                "status"]
        )

        nodes = self.manager.ListNodes()
        for node in nodes:
            info = NodeInfo(*node)
            print(f"Node: {info.name}, State: {info.status}")

    def SetCPUWeight(
        self,
        nodeName,
        unitName,
        value,
        persist=False
    ):
        """
        Set CPU Weight

        Args:
            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name

            value:
                value to be set

            persist:
                Set True to NOT persist de change
                Default value is False

        Returns:
        """
        # Don't persist change = True
        runtime = persist

        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        node.SetUnitProperties(
            unitName,
            runtime,
            [("CPUWeight", Variant("t", value))]
        )

    def CPUWeight(
        self,
        nodeName,
        unitName
    ):
        """
        Get the CPU Weight for the unitName

        Args:
            nodeName:
                The name of node to execute the operation

            unitName:
                Unit Name

        Returns:
        """
        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        print(
            node.GetUnitProperty(
                unitName,
                "org.freedesktop.systemd1.Service",
                "CPUWeight"
            )
        )

    def ListActiveServices(
        self,
    ):
        """
        List all Active services in the nodes

        Args:

        Returns:
        """
        NodeUnitInfo = namedtuple(
           "NodeUnitInfo", [
               "node",
               "name",
               "description",
               "load_state",
               "active_state",
               "sub_state",
               "follower",
               "object_path",
               "job_id",
               "job_type",
               "job_object_path"
           ]
        )

        units = self.manager.ListUnits()
        for u in units:
            info = NodeUnitInfo(*u)
            if info.active_state == "active" and \
                    info.name.endswith(".service"):
                print(f"Node: {info.node}, Unit: {info.name}")

    def ListNodeUnits(
        self,
        nodeName=None
    ):
        """
        List Units in the node

        Args:
            nodeName:
                The name of node to execute the operation

        Returns:
        """

        if nodeName is None:
            return namedtuple("UnitInfo", [])

        UnitInfo = namedtuple(
            "UnitInfo", [
                "name",
                "description",
                "load_state",
                "active_state",
                "sub_state",
                "follower",
                "object_path",
                "job_id",
                "job_type",
                "job_object_path"
            ]
        )

        node_path = self.manager.GetNode(nodeName)
        node = self.bus.get_proxy(
            HIRTE_DBUS_INTERFACE,
            node_path
        )

        units = node.ListUnits()
        for u in units:
            info = UnitInfo(*u)
            print(f"{info.name} - {info.description}")
