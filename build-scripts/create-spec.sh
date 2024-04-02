#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

VERSION_SCRIPT=$(dirname "$(readlink -f "$0")")/version.sh

VERSION="$(${VERSION_SCRIPT} short)"
RELEASE="$(${VERSION_SCRIPT} release)"

# Set version and release
sed \
    -e "s|@VERSION@|${VERSION}|g" \
    -e "s|@RELEASE@|${RELEASE}|g" \
    < bluechi.spec.in \
    > bluechi.spec
