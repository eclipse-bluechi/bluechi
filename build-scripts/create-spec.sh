#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

VERSION="$($(dirname "$(readlink -f "$0")")/get-version.sh)"

# Mark current directory as safe for git to be able to parse git hash
git config --global --add safe.directory $(pwd)

# Package release
#
# Use following for official releases
#RELEASE="1"
# Use following for nightly builds
RELEASE="0.$(date +%04Y%02m%02d%02H%02M).git$(git rev-parse --short ${GITHUB_SHA:-HEAD})"


# Set version and release
sed \
    -e "s|@VERSION@|${VERSION}|g" \
    -e "s|@RELEASE@|${RELEASE}|g" \
    < bluechi.spec.in \
    > bluechi.spec
