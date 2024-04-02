#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Tuple
from dasbus.typing import DBusType, Byte, get_type_name


DBUS_REPRESENTATION_ARRAY = "a"
DBUS_REPRESENTATION_STRUCT_OPEN = "("
DBUS_REPRESENTATION_STRUCT_CLOSE = ")"
DBUS_REPRESENTATION_DICT_OPEN = "{"
DBUS_REPRESENTATION_DICT_CLOSE = "}"


class DBusTypeExtended(DBusType):
    def __init__(self) -> None:
        super().__init__()

        self._basic_type_mapping_reversed = dict()
        for key, value in self._basic_type_mapping.items():
            self._basic_type_mapping_reversed[value] = key

    def is_dbus_char_basic_type(self, char: str) -> bool:
        return char in self._basic_type_mapping_reversed

    def _map_basic_dbus_type_char_to_pytype_string(self, type_char: str) -> str:
        if type_char not in self._basic_type_mapping_reversed:
            raise Exception(f"Unknown basic type '{type_char}'")

        return get_type_name(self._basic_type_mapping_reversed[type_char])

    def parse_dbus_type_string(self, type_string: str) -> str:
        """ "parse a D-Bus API string to a python type string (e.g. "s(ss)" -> "Tuple[str, List[str, str]]")

        Parameters:
        type_string: The D-Bus API string to parse

        Returns:
        str: The parsed python type string
        """
        if len(type_string) < 1:
            return "", 0
        if len(type_string) == 1:
            return self._map_basic_dbus_type_char_to_pytype_string(type_string)

        if type_string[0] == DBUS_REPRESENTATION_STRUCT_OPEN:
            return self._map_dbus_struct_to_pytype_string(type_string)[0]
        elif type_string[0] == DBUS_REPRESENTATION_ARRAY:
            return self._map_dbus_array_type_to_pytype_string(type_string)[0]
        return self._map_dbus_struct_to_pytype_string(f"({type_string})")[0]

    def _map_dbus_array_type_to_pytype_string(
        self, type_string: str
    ) -> Tuple[str, int]:
        """ "Internal function. Parses a D-Bus API array string.

        Parameters:
        type_string: The D-Bus API string to parse. Must be an array, starting with DBUS_REPRESENTATION_ARRAY

        Returns:
        Tuple: The parsed python type string and the number of characters read from the string
        """
        if len(type_string) < 1 or type_string[0] != DBUS_REPRESENTATION_ARRAY:
            raise Exception(
                "Invalid type definition for array detected in '{t}' - No 'a' found!"
            )

        if type_string[1] == DBusType.get_dbus_representation(Byte):
            # chars_read = 2
            #    since the chars "ay" have been read
            chars_read = 2

            return "bytes", chars_read
        elif self.is_dbus_char_basic_type(type_string[1]):
            # chars_read = 2
            #    since the chars "a." have been read
            #    with "." being any basic type
            chars_read = 2

            return (
                f"List[{self._map_basic_dbus_type_char_to_pytype_string(type_string[1])}]",
                chars_read,
            )
        elif type_string[1] == DBUS_REPRESENTATION_STRUCT_OPEN:
            mapped, chars_read = self._map_dbus_struct_to_pytype_string(type_string[1:])

            # chars_read + 3
            #    since the chars "a()" have been read, in addition to the content of ()
            return f"List[{mapped}]", chars_read + 3
        elif type_string[1] == DBUS_REPRESENTATION_DICT_OPEN:
            if not self.is_dbus_char_basic_type(type_string[2]):
                raise Exception(
                    f"Invalid dict definition for '{type_string}' - key must be a basic type"
                )
            key = self._map_basic_dbus_type_char_to_pytype_string(type_string[2])

            if (
                self.is_dbus_char_basic_type(type_string[3])
                and type_string[4] == DBUS_REPRESENTATION_DICT_CLOSE
            ):
                value = self._map_basic_dbus_type_char_to_pytype_string(type_string[3])
                ret = f"Dict[{key}, {value}]"
                if ret == "Dict[str, Variant]":
                    ret = "Structure"

                # chars_read = 5
                #    since the chars "a{..}" have been read
                #    with "." being any basic type
                chars_read = 5

                return ret, chars_read
            if type_string[3] == DBUS_REPRESENTATION_ARRAY:
                mapped, chars_read = self._map_dbus_array_type_to_pytype_string(
                    type_string[3:]
                )

                # chars_read + 4
                #    since the chars "a{.a}" have been read
                #    with "." being any basic type
                #    (excluding the array in the value)
                return f"Dict[{key}, {mapped}]", chars_read + 4
            if type_string[3] == DBUS_REPRESENTATION_STRUCT_OPEN:
                mapped, chars_read = self._map_dbus_struct_to_pytype_string(
                    type_string[3:]
                )

                # chars_read + 6
                #    since the chars "a{.()}" have been read
                #    with "." being any basic type
                #    (excluding the content of the struct)
                return f"Dict[{key}, {mapped}]", chars_read + 6

            raise Exception(
                f"Invalid dictionary detected in '{type_string}' - Dict must contain exactly 2 arguments!"
            )

    def _map_dbus_struct_to_pytype_string(self, type_string: str) -> Tuple[str, int]:
        """ "Internal function. Parses a D-Bus API struct string.

        Parameters:
        type_string: The D-Bus API string to parse. Must be a struct, starting with DBUS_REPRESENTATION_STRUCT_OPEN

        Returns:
        Tuple: The parsed python type string and the number of characters read from the string
        """
        if len(type_string) < 1 or type_string[0] != DBUS_REPRESENTATION_STRUCT_OPEN:
            raise Exception(
                f"Invalid type definition for struct detected in '{type_string}' - No '(' found!"
            )

        found_types = []
        j = -1
        i = 1
        while i < len(type_string):
            c = type_string[i]
            if c == DBUS_REPRESENTATION_STRUCT_CLOSE:
                j = i - 1
                break

            if self.is_dbus_char_basic_type(c):
                found_types.append(self._map_basic_dbus_type_char_to_pytype_string(c))
                i += 1
            elif c == DBUS_REPRESENTATION_STRUCT_OPEN:
                mapped, chars_read = self._map_dbus_struct_to_pytype_string(
                    type_string[i:]
                )

                # chars_read + 2
                #    since the chars "()" have been read
                i += chars_read + 2
                found_types.append(mapped)
            elif c == DBUS_REPRESENTATION_ARRAY:
                mapped, chars_read = self._map_dbus_array_type_to_pytype_string(
                    type_string[i:]
                )
                i += chars_read
                found_types.append(mapped)
            else:
                raise Exception(f"Invalid type definition in struct detected '{c}'")

        if j == -1:
            raise Exception(
                f"Invalid type definition for struct detected in '{type_string}' - No ')' found!"
            )

        return f"Tuple[{', '.join(found_types)}]", j
