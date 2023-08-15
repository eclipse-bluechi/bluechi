# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import List

BASE_INTERFACE_NAME = "org.eclipse.bluechi"


class Interface:
    def __init__(self, path: str) -> None:
        self.path: str = path
        self.name: str = path.replace(BASE_INTERFACE_NAME + ".", "")
        self.methods: List[Method] = []
        self.signals: List[Signal] = []
        self.properties: List[Property] = []


class Method:
    def __init__(self, name: str) -> None:
        self.name: str = name
        self.args: List[MethodArg] = []


class MethodArg:
    def __init__(self, name: str, direction: str, argtype: str) -> None:
        self.name: str = name
        self.direction: str = direction
        self.type: str = argtype


class Signal:
    def __init__(self, name: str) -> None:
        self.name: str = name
        self.args: List[SignalArg] = []


class SignalArg:
    def __init__(self, name: str, argtype: str) -> None:
        self.name: str = name
        self.type: str = argtype


class Property:
    def __init__(self, name: str, proptype: str, access: str) -> None:
        self.name: str = name
        self.type: str = proptype
        self.access: str = access
