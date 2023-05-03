#!/usr/bin/python3

from collections import namedtuple
import dasbus.connection
bus = dasbus.connection.SystemMessageBus()

NodeUnitInfo = namedtuple("NodeUnitInfo", ["node", "name",
                                           "description", "load_state", "active_state", "sub_state", "follower",
                                           "object_path", "job_id", "job_type", "job_object_path"])

manager = bus.get_proxy("org.containers.hirte",  "/org/containers/hirte")
units = manager.ListUnits()
for u in units:
    info = NodeUnitInfo(*u)
    if info.active_state == "active" and info.name.endswith(".service"):
        print(f"Node: {info.node}, Unit: {info.name}")
