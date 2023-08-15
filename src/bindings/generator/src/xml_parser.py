# SPDX-License-Identifier: GPL-2.0-or-later

from os import listdir
from os.path import join
from typing import List

import xml.etree.ElementTree as ET
from xml.etree.ElementTree import Element

from model import Interface, Method, MethodArg, Signal, SignalArg, Property

API_FILENAME_PREFIX = "org.eclipse.bluechi"
API_EXTENSION = ".xml"


def is_file_internal_api(filename: str) -> bool:
    return ".internal." in filename


def list_api_files(dir: str) -> List[str]:
    files = []

    for file in listdir(dir):
        if (
            file.startswith(API_FILENAME_PREFIX)
            and file.endswith(API_EXTENSION)
            and not is_file_internal_api(file)
        ):
            files.append(join(dir, file))

    return files


def parse_api_file(file: str) -> List[Interface]:
    root = ET.parse(file).getroot()
    if root.tag != "node":
        raise Exception("Invalid API xml: '<node>' has to be root tag")
    return parse_node_tag(root)


def parse_node_tag(node: Element) -> List[Interface]:
    interfaces = []
    for interface in node.findall("interface"):
        interfaces.append(parse_interface_tag(interface))
    return interfaces


def parse_interface_tag(interface: Element) -> Interface:
    iname = interface.attrib["name"]

    iface = Interface(iname)

    for method in interface.findall("method"):
        parse_method_tag(method, iface)
    for signal in interface.findall("signal"):
        parse_signal_tag(signal, iface)
    for property in interface.findall("property"):
        parse_property_tag(property, iface)

    return iface


def parse_method_tag(method: Element, interface: Interface):
    m = Method(method.attrib["name"])
    for arg in method.findall("arg"):
        m.args.append(
            MethodArg(arg.attrib["name"], arg.attrib["direction"], arg.attrib["type"])
        )
    interface.methods.append(m)


def parse_signal_tag(signal: Element, interface: Interface):
    s = Signal(signal.attrib["name"])
    for arg in signal.findall("arg"):
        s.args.append(SignalArg(arg.attrib["name"], arg.attrib["type"]))
    interface.signals.append(s)


def parse_property_tag(property: Element, interface: Interface):
    interface.properties.append(
        Property(
            property.attrib["name"], property.attrib["type"], property.attrib["access"]
        )
    )
