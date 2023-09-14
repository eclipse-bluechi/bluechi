#!/usr/bin/env python
# SPDX-License-Identifier: MIT-0

from bluechi.ext import Unit

result = Unit("my-node-name").stop_unit("chronyd.service")
print(result)
