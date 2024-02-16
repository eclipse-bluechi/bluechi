#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

set -x
dnf install \
    podman \
    python3-dasbus \
    python3-pytest \
    python3-pytest-timeout \
    -y
