# SPDX-License-Identifier: LGPL-2.1-or-later

import configparser
import enum
import io

from typing import Union


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

    # Available options for Install section can be found at
    # https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html#%5BInstall%5D%20Section%20Options
    WantedBy = "WantedBy"


class Service():
    """An empty service, which only contains predefined sections"""
    def __init__(self, name: str = None) -> None:
        self.cp = configparser.ConfigParser()
        # Enable case sensitivity
        self.cp.optionxform = lambda option: option
        self.name = name
        for section in Section:
            self.cp.add_section(section.value)

    def __str__(self) -> str:
        with io.StringIO() as content:
            self.cp.write(content, space_around_delimiters=False)
            return content.getvalue()

    def set_option(self, section: Section, name: Option, value: str) -> None:
        self.cp.set(section.value, name.value, value)

    def get_option(self, section: Section, name: Option) -> Union[str, None]:
        if self.cp.has_option(section.value, name.value):
            return self.cp.get(section.value, name.value)

        return None

    def remove_option(self, section: Section, name: Option) -> None:
        if self.cp.has_option(section.value, name.value):
            self.cp.remove_option(section.value, name.value)


class SimpleService(Service):
    """A simple service, which is using /bin/true command"""
    def __init__(self, name: str = "simple.service") -> None:
        super(SimpleService, self).__init__(name)

        self.set_option(Section.Unit, Option.Description, "A simple service")

        self.set_option(Section.Service, Option.Type, "simple")
        self.set_option(Section.Service, Option.ExecStart, "/bin/true")

        self.set_option(Section.Install, Option.WantedBy, "multi-user.target")


class SimpleRemainingService(SimpleService):
    """A simple service, which adds RemainAfterExit enabled to SimpleService"""
    def __init__(self, name: str = "simple.service") -> None:
        super(SimpleRemainingService, self).__init__(name)

        self.set_option(Section.Service, Option.RemainAfterExit, "yes")


class SleepingService(Service):
    """A simple service, which is using /bin/sleep command"""
    def __init__(self, name: str = "sleeping.service") -> None:
        super(SleepingService, self).__init__(name)

        self.set_option(Section.Unit, Option.Description, "A sleeping service")

        self.set_option(Section.Service, Option.Type, "simple")
        self.set_option(Section.Service, Option.ExecStart, "/bin/sleep 60")

        self.set_option(Section.Install, Option.WantedBy, "multi-user.target")
