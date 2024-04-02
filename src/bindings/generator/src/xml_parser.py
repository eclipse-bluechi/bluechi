#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

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
    xml_parser_with_comments = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    root = ET.parse(file, parser=xml_parser_with_comments).getroot()
    if root.tag != "node":
        raise Exception("Invalid API xml: '<node>' has to be root tag")
    return parse_node_tag(root)


def is_comment(elem: Element) -> bool:
    return "function Comment" in str(elem.tag)


def is_interface(elem: Element) -> bool:
    return str(elem.tag) == "interface"


def is_method(elem: Element) -> bool:
    return str(elem.tag) == "method"


def is_signal(elem: Element) -> bool:
    return str(elem.tag) == "signal"


def is_property(elem: Element) -> bool:
    return str(elem.tag) == "property"


def does_property_emit_changed_signal(elem: Element) -> bool:
    for annotation in elem.findall("annotation"):
        if annotation.attrib["name"] == "org.freedesktop.DBus.Property.EmitsChangedSignal" \
                and annotation.attrib["value"] == "true":
            return True
    return False


def parse_node_tag(node: Element) -> List[Interface]:
    interfaces = []

    doc_comment = None
    for elem in node.iter():
        if is_interface(elem):
            interfaces.append(parse_interface_tag(elem, doc_comment))
        if doc_comment is not None:
            doc_comment = None
        if is_comment(elem):
            doc_comment = elem
    return interfaces


def parse_interface_tag(interface: Element, doc_comment: Element) -> Interface:
    iname = interface.attrib["name"]

    iface = Interface(iname, "" if doc_comment is None else doc_comment.text)

    doc_comment = None
    for elem in interface.iter():
        if is_method(elem):
            parse_method_tag(elem, doc_comment, iface)
        if is_signal(elem):
            parse_signal_tag(elem, doc_comment, iface)
        if is_property(elem):
            parse_property_tag(elem, doc_comment, iface)
        if doc_comment is not None:
            doc_comment = None
        if is_comment(elem):
            doc_comment = elem

    return iface


def parse_method_tag(method: Element, doc_comment: Element, interface: Interface):
    m = Method(method.attrib["name"], "" if doc_comment is None else doc_comment.text)
    for arg in method.findall("arg"):
        m.args.append(
            MethodArg(arg.attrib["name"], arg.attrib["direction"], arg.attrib["type"])
        )
    interface.methods.append(m)


def parse_signal_tag(signal: Element, doc_comment: Element, interface: Interface):
    s = Signal(signal.attrib["name"], "" if doc_comment is None else doc_comment.text)
    for arg in signal.findall("arg"):
        s.args.append(SignalArg(arg.attrib["name"], arg.attrib["type"]))
    interface.signals.append(s)


def parse_property_tag(property: Element, doc_comment: Element, interface: Interface):
    interface.properties.append(
        Property(
            property.attrib["name"],
            "" if doc_comment is None else doc_comment.text,
            property.attrib["type"],
            property.attrib["access"],
            does_property_emit_changed_signal(property),
        )
    )
