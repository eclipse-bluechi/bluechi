#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import List

BASE_INTERFACE_NAME = "org.eclipse.bluechi"


class Interface:
    def __init__(self, path: str, inline_doc: str) -> None:
        self.path: str = path
        self.name: str = path.replace(BASE_INTERFACE_NAME + ".", "")
        self.inline_doc: str = inline_doc.strip()

        self.methods: List[Method] = []
        self.signals: List[Signal] = []
        self.properties: List[Property] = []


class Method:
    def __init__(self, name: str, inline_doc: str) -> None:
        self.name: str = name
        self.inline_doc: str = inline_doc.strip()

        self.args: List[MethodArg] = []


class MethodArg:
    def __init__(self, name: str, direction: str, argtype: str) -> None:
        self.name: str = name
        self.direction: str = direction
        self.type: str = argtype


class Signal:
    def __init__(self, name: str, inline_doc: str) -> None:
        self.name: str = name
        self.inline_doc: str = inline_doc.strip()

        self.args: List[SignalArg] = []


class SignalArg:
    def __init__(self, name: str, argtype: str) -> None:
        self.name: str = name
        self.type: str = argtype


class Property:
    def __init__(self, name: str, inline_doc: str, proptype: str, access: str, emits_change: bool) -> None:
        self.name: str = name
        self.inline_doc: str = inline_doc.strip()
        self.type: str = proptype
        self.access: str = access
        self.emits_change: bool = emits_change
