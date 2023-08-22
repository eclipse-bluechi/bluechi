#!/bin/bash -e
# SPDX-License-Identifier: LGPL-2.1-or-later

# Parse package version from the project
meson setup builddir 1>&2
VERSION="$(meson introspect --projectinfo builddir | jq -r '.version')"

echo ${VERSION}