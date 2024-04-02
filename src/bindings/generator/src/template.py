#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import datetime
from typing import List, Dict, Any
from jinja2 import Environment, FileSystemLoader

from model import Interface
from dbus_typing import DBusTypeExtended


def pascal_to_snake(s):
    return "".join(["_" + c.lower() if c.isupper() else c for c in s]).lstrip("_")


def model_to_data_dict(interfaces: List[Interface]) -> Dict[str, Any]:
    types = DBusTypeExtended()

    data = {"interfaces": []}
    for iface in interfaces:
        data_interface = {
            "name": iface.name,
            "inline_doc": iface.inline_doc,
            "methods": [],
            "signals": [],
            "properties": [],
        }

        methods = []
        for method in iface.methods:
            data_method = {
                "name": method.name,
                "inline_doc": method.inline_doc,
                "pyname": pascal_to_snake(method.name),
                "args": [],
                "rets": [],
            }
            for arg in method.args:
                e = {
                    "name": arg.name,
                    "pytype": types.parse_dbus_type_string(arg.type),
                }
                if arg.direction == "in":
                    data_method["args"].append(e)
                elif arg.direction == "out":
                    data_method["rets"].append(e)

            methods.append(data_method)
        data_interface["methods"] = sorted(methods, key=lambda item: item["name"])

        signals = []
        for signal in iface.signals:
            data_signal = {
                "name": signal.name,
                "inline_doc": signal.inline_doc,
                "pyname": pascal_to_snake(signal.name),
                "args": [],
            }
            for arg in signal.args:
                e = {
                    "name": arg.name,
                    "pytype": types.parse_dbus_type_string(arg.type),
                }
                data_signal["args"].append(e)

            signals.append(data_signal)
        data_interface["signals"] = sorted(signals, key=lambda item: item["name"])

        props = []
        for prop in iface.properties:
            e = {
                "name": prop.name,
                "inline_doc": prop.inline_doc,
                "pyname": pascal_to_snake(prop.name),
                "pytype": types.parse_dbus_type_string(prop.type),
                "access": prop.access.split("|"),
                "emits_change": prop.emits_change,
            }
            props.append(e)
        data_interface["properties"] = sorted(props, key=lambda item: item["name"])

        data["interfaces"].append(data_interface)

    data["now"] = datetime.datetime.utcnow
    data["interfaces"] = sorted(data["interfaces"], key=lambda item: item["name"])
    return data


def render(template: str, template_dir: str, data: Dict[str, Any]) -> str:
    tmpl = Environment(loader=FileSystemLoader(template_dir)).get_template(template)
    return tmpl.render(data)
