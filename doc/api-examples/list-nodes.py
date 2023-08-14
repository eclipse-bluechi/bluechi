#!/usr/bin/python3

from collections import namedtuple
import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

NodeInfo = namedtuple("NodeInfo", ["name", "object_path", "status"])

manager = bus.get_proxy("io.github.eclipse-bluechi.bluechi", "/io/github/eclipse_bluechi/bluechi")
nodes = manager.ListNodes()
for n in nodes:
    info = NodeInfo(*n)
    print(f"Node: {info.name}, State: {info.status}")
