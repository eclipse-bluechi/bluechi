#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
from typing import AnyStr, Dict

# Literals used during bluechictl/systemctl output parsing
_ATTR_NODE_NAME = "node_name"
_ATTR_UNIT_NAME = "unit_name"
_ATTR_UNIT_FILE = "unit_file"
_ATTR_PRESET = "preset"
_ATTR_STATE = "state"
_ATTR_SUB_STATE = "sub_state"
_ATTR_LOADED = "loaded"
_ATTR_DESCRIPTION = "description"


class RegexPattern:
    # Pattern to parse bluechictl list-units line
    BLUECHITL_LIST_UNITS = re.compile(
        rf"""\s*(?P<{_ATTR_NODE_NAME}>[\S]+)\s*\|
            \s*(?P<{_ATTR_UNIT_NAME}>[\S]+)\s*\|
            \s*(?P<{_ATTR_STATE}>[\S]+)\s*\|
            \s*(?P<{_ATTR_SUB_STATE}>[\S]+)\s*""",
        re.VERBOSE,
    )

    # Pattern to parse bluechictl list-unit-files line
    BLUECHICTL_LIST_UNIT_FILES = re.compile(
        rf"""\s*(?P<{_ATTR_NODE_NAME}>[\S]+)\s*\|
            \s*((?:[^/]*/)*)(?P<{_ATTR_UNIT_FILE}>[\S]+)\s*\|
            \s*(?P<{_ATTR_STATE}>[\S]+)\s*""",
        re.VERBOSE,
    )

    # Pattern to parse systemctl list-units line
    SYSTEMCTL_LIST_UNITS = re.compile(
        rf"""\s*(?P<{_ATTR_UNIT_NAME}>\S+)
            \s+(?P<{_ATTR_LOADED}>\S+)
            \s+(?P<{_ATTR_STATE}>\S+)
            \s+(?P<{_ATTR_SUB_STATE}>\S+)
            \s+(?P<{_ATTR_DESCRIPTION}>.*)$""",
        re.VERBOSE,
    )

    # Pattern to parse systemctl list-unit-files line
    SYSTEMCTL_LIST_UNIT_FILES = re.compile(
        rf"""\s*(?P<{_ATTR_UNIT_FILE}>\S+)
            \s+(?P<{_ATTR_STATE}>\S+)
            \s+(?P<{_ATTR_PRESET}>\S+)""",
        re.VERBOSE,
    )

    # Pattern to remove escape sequences appearing on some systemctl output lines
    SC_LINE_ESC_SEQ = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")


class SystemdListItem:
    def __init__(
        self,
        key: str,
    ):
        self.key = key

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        return self.key == other.key

    def __hash__(self):
        return hash(self.key)

    def __str__(self):
        return f"SystemdListItem(" f"key='{self.key}')"

    def attributeEquals(self, other) -> bool:
        return self.__eq__(other)

    @classmethod
    def from_match(cls, match: re.Match):
        raise NotImplementedError


class SystemdUnit(SystemdListItem):
    def __init__(
        self,
        name: str,
        state: str = None,
        sub_state: str = None,
        description: str = None,
    ):
        super().__init__(name)
        self.state = state
        self.sub_state = sub_state
        self.description = description

    def __str__(self):
        return (
            f"SystemdUnit("
            f"name='{self.key}', "
            f"state='{self.state}', "
            f"sub_state='{self.sub_state}', "
            f"description='{self.description}')"
        )

    def attributeEquals(self, other) -> bool:
        if not isinstance(other, self.__class__):
            return False
        return self.state == other.state and self.sub_state == other.sub_state

    @classmethod
    def from_match(cls, match: re.Match):
        return cls(
            name=match.group(_ATTR_UNIT_NAME),
            state=match.group(_ATTR_STATE),
            sub_state=match.group(_ATTR_SUB_STATE),
            description=(
                match.group(_ATTR_DESCRIPTION)
                if _ATTR_DESCRIPTION in match.groupdict()
                else ""
            ),
        )


class SystemdUnitFile(SystemdListItem):
    def __init__(
        self,
        file_name: str,
        state: str = None,
        preset: str = None,
    ):
        super().__init__(file_name)
        self.state = state
        self.preset = preset

    def __str__(self):
        return (
            f"SystemdUnitFile("
            f"file_name='{self.key}', "
            f"state='{self.state}', "
            f"preset='{self.preset}')"
        )

    def attributeEquals(self, other) -> bool:
        if not isinstance(other, self.__class__):
            return False
        return self.state == other.state

    @classmethod
    def from_match(cls, match: re.Match):
        return cls(
            file_name=match.group(_ATTR_UNIT_FILE),
            state=match.group(_ATTR_STATE),
            preset=(
                match.group(_ATTR_PRESET) if _ATTR_PRESET in match.groupdict() else ""
            ),
        )


# Parse output of bluechictl list-* and return it as a dictionary containing list of items per each node
def parse_bluechictl_list_output(
    content: str,
    line_pattern: re.Pattern[AnyStr],
    item_class: SystemdListItem,
) -> Dict[str, Dict[str, SystemdListItem]]:
    result = {}
    for line in content.splitlines():
        if line.startswith("NODE ") or line.startswith("===="):
            # Ignore header lines
            continue

        match = line_pattern.match(line)
        if not match:
            raise Exception(f"Error parsing bluechictl output, invalid line: '{line}'")

        node_items = result.get(match.group(_ATTR_NODE_NAME))
        if not node_items:
            node_items = {}
            result[match.group(_ATTR_NODE_NAME)] = node_items

        item = item_class.from_match(match)

        if item.key in node_items:
            raise Exception(
                f"Error parsing bluechictl output, item already reported, line: '{line}'"
            )

        node_items[item.key] = item

    return result


# Parse output of systemctl list-* and return it as list items
def parse_systemctl_list_output(
    content: str, line_pattern: re.Pattern[AnyStr], item_class: SystemdListItem
) -> Dict[str, SystemdListItem]:
    result = {}
    for raw_line in content.splitlines():
        # Some systemctl output contains ANSI sequences, which we need to remove before matching
        line = RegexPattern.SC_LINE_ESC_SEQ.sub("", raw_line)

        match = line_pattern.match(line)
        if not match:
            raise Exception(f"Error parsing systemctl output, invalid line: '{line}'")

        item = item_class.from_match(match)

        if item.key in result:
            raise Exception(
                f"Error parsing systemctl output, item already reported, line: '{line}'"
            )

        result[item.key] = item

    return result


# Check if all items and their attributes reported by bluechictl for specific node aligns with systemctl
# executed on the same node
def compare_lists(
    bc_items: Dict[str, SystemdListItem],
    sc_items: Dict[str, SystemdListItem],
    node_name: str,
):

    for sc_item in sc_items.values():
        bc_item = bc_items.get(sc_item.key)
        if not bc_item:
            raise Exception(
                f"Item '{sc_item}' is reported by systemctl on node '{node_name}', but not by bluechictl"
            )
        if not sc_item.attributeEquals(bc_item):
            raise Exception(
                f"Attribute values reported by bluechictl '{bc_item}' differs from attribute values"
                f" reported by systemctl '{sc_item}' on node '{node_name}"
            )

    for bc_item in bc_items.values():
        if bc_item.key not in sc_items:
            raise Exception(
                f"Item '{bc_item}' is reported by bluechictl, but not by systemctl on node '{node_name}'"
            )
