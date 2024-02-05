# SPDX-License-Identifier: LGPL-2.1-or-later

import logging

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class BlueChiCtl():

    binary_name = "bluechictl"

    def __init__(self, client: Client) -> None:
        self.client = client

    def start_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Starting unit '{unit_name}' on node '{node_name}'")

        result, output = self.client.exec_run(f"{BlueChiCtl.binary_name} start {node_name} {unit_name}")
        LOGGER.debug(f"Start unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def stop_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Stopping unit '{unit_name}' on node '{node_name}'")

        result, output = self.client.exec_run(f"{BlueChiCtl.binary_name} stop {node_name} {unit_name}")
        LOGGER.debug(f"Stop unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to start service {unit_name} on node {node_name}: {output}")

    def enable_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Enabling unit '{unit_name}' on node '{node_name}'")

        result, output = self.client.exec_run(f"{BlueChiCtl.binary_name} enable {node_name} {unit_name}")
        LOGGER.debug(f"Enable unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to enable service {unit_name} on node {node_name}: {output}")

    def freeze_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Freezing unit {unit_name} on node {node_name}")

        result, output = self.client.exec_run(f"{BlueChiCtl.binary_name} freeze {node_name} {unit_name}")
        LOGGER.debug(f"Freeze unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to freeze service {unit_name} on node {node_name}: {output}")

    def thaw_unit(self, node_name: str, unit_name: str) -> None:
        LOGGER.debug(f"Thawing unit {unit_name} on node {node_name}")

        result, output = self.client.exec_run(f"{BlueChiCtl.binary_name} thaw {node_name} {unit_name}")
        LOGGER.debug(f"Thaw unit '{unit_name}' on node '{node_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to thaw service {unit_name} on node {node_name}: {output}")
