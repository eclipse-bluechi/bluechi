#!/usr/bin/env python
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from bluechi.ext import Unit

result = Unit("my-node-name").start_unit("chronyd.service")
print(result)
