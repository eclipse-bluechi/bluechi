# SPDX-License-Identifier: LGPL-2.1-or-later

import pytest


pytest_plugins = [
    "bluechi_test.fixtures"
]


# Set minimum timeout for all tests to 45 seconds.
# If some test needs bigger timeout, please override it for the specific test
# using @pytest.mark.timeout annotation
def pytest_collection_modifyitems(items):
    for item in items:
        if item.get_closest_marker('timeout') is None:
            item.add_marker(pytest.mark.timeout(45))
