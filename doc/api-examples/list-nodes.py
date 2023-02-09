#!/usr/bin/python3

from collections import namedtuple
import dasbus.connection

bus = dasbus.connection.SessionMessageBus()

NodeInfo = namedtuple("NodeInfo", ["name", "object_path", "status"])

manager = bus.get_proxy("org.containers.hirte",  "/org/containers/hirte")
nodes = manager.ListNodes()
for n in nodes:
    info = NodeInfo(*n)
    print(f"Node: {info.name}, State: {info.status}")
