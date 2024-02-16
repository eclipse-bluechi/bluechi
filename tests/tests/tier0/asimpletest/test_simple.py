# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import logging


LOGGER = logging.getLogger(__name__)


def test_simple():
    LOGGER.info("")
    LOGGER.info("")

    LOGGER.info("------------------------------------------------------")
    LOGGER.info("------------------------------------------------------")
    LOGGER.info("------------------------------------------------------")
    topology_file_path = os.getenv("TMT_TOPOLOGY_BASH")
    if topology_file_path is None:
        LOGGER.info("No env variable TMT_TOPOLOGY_BASH found")
        return

    with open(topology_file_path, "r") as f:
        LOGGER.info(f.read())
    LOGGER.info("")
    LOGGER.info("")
    LOGGER.info("------------------------------------------------------")
    LOGGER.info(os.environ)
    LOGGER.info("")
    LOGGER.info("")
    LOGGER.info("------------------------------------------------------")
    LOGGER.info("------------------------------------------------------")
    LOGGER.info("------------------------------------------------------")
    LOGGER.info("")
    LOGGER.info("")
