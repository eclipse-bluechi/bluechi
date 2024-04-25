#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import configparser
import enum
import io
import pathlib
from typing import Union

# Default service installation directory
SERVICE_DIRECTORY = "/etc/systemd/system"

# Default service name (it should be changed in child classes)
SERVICE_NAME = "empty.service"


@enum.unique
class Section(enum.Enum):
    # Available sections are described at https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html
    Install = "Install"
    Service = "Service"
    Unit = "Unit"


@enum.unique
class Option(enum.Enum):
    # Available options for Unit section can be found at
    # https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html#%5BUnit%5D%20Section%20Options
    After = "After"
    Description = "Description"
    StopWhenUnneeded = "StopWhenUnneeded"
    Wants = "Wants"
    BindsTo = "BindsTo"

    # Available options for Service section can be found at
    # https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#Options
    ExecReload = "ExecReload"
    ExecStart = "ExecStart"
    ExecStartPre = "ExecStartPre"
    RemainAfterExit = "RemainAfterExit"
    Type = "Type"
    User = "User"
    Group = "Group"

    # Available options for Install section can be found at
    # https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html#%5BInstall%5D%20Section%20Options
    WantedBy = "WantedBy"


class Service:
    """A base service class, which by default contains only predefined sections"""

    def __init__(
        self, directory: str = SERVICE_DIRECTORY, name: str = SERVICE_NAME
    ) -> None:
        if not name:
            raise Exception(f"Service name is '{name}', but cannot be empty!")
        if not directory:
            raise Exception(f"Service directory is '{directory}', but cannot be empty!")
        self._svc_path = pathlib.Path(directory, name)

        self._cfg_parser = self._init_parser()
        for section in Section:
            self._cfg_parser.add_section(section.value)

    def _init_parser(self) -> configparser.ConfigParser:
        cp = configparser.ConfigParser()
        # Enable case sensitivity
        cp.optionxform = lambda option: option
        return cp

    @property
    def path(self) -> pathlib.Path:
        return self._svc_path

    @property
    def name(self) -> str:
        return str(self._svc_path.name)

    @property
    def directory(self) -> str:
        return str(self._svc_path.parent)

    def set_option(self, section: Section, name: Option, value: str) -> None:
        self._cfg_parser.set(section.value, name.value, value)

    def get_option(self, section: Section, name: Option) -> Union[str, None]:
        if self._cfg_parser.has_option(section.value, name.value):
            return self._cfg_parser.get(section.value, name.value)
        return None

    def remove_option(self, section: Section, name: Option) -> None:
        if self._cfg_parser.has_option(section.value, name.value):
            self._cfg_parser.remove_option(section.value, name.value)

    def to_string(self) -> str:
        with io.StringIO() as content:
            self._cfg_parser.write(content, space_around_delimiters=False)
            return content.getvalue()

    def from_string(self, content: str) -> None:
        self._cfg_parser = self._init_parser()
        self._cfg_parser.read_string(content)


class SimpleService(Service):
    """A simple service, which is using /bin/true command"""

    def __init__(
        self, directory: str = SERVICE_DIRECTORY, name: str = "simple.service"
    ) -> None:
        super(SimpleService, self).__init__(directory=directory, name=name)

        self.set_option(Section.Unit, Option.Description, "A simple service")

        self.set_option(Section.Service, Option.Type, "simple")
        self.set_option(Section.Service, Option.ExecStart, "/bin/true")

        self.set_option(Section.Install, Option.WantedBy, "multi-user.target")


class SimpleRemainingService(SimpleService):
    """A simple service, which adds RemainAfterExit enabled to SimpleService"""

    def __init__(
        self, directory: str = SERVICE_DIRECTORY, name: str = "simple.service"
    ) -> None:
        super(SimpleRemainingService, self).__init__(directory=directory, name=name)

        self.set_option(Section.Service, Option.RemainAfterExit, "yes")


class SleepingService(Service):
    """A simple service, which is using /bin/sleep command"""

    def __init__(
        self, directory: str = SERVICE_DIRECTORY, name: str = "sleeping.service"
    ) -> None:
        super(SleepingService, self).__init__(directory=directory, name=name)

        self.set_option(Section.Unit, Option.Description, "A sleeping service")

        self.set_option(Section.Service, Option.Type, "simple")
        self.set_option(Section.Service, Option.ExecStart, "/bin/sleep 60")

        self.set_option(Section.Install, Option.WantedBy, "multi-user.target")
