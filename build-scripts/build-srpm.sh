#!/bin/bash -xe
# SPDX-License-Identifier: GPL-2.0-or-later

# Parse package version from the project
meson setup builddir
VERSION="$(meson introspect --projectinfo builddir | jq -r '.version')"

# Mark current directory as safe for git to be able to parse git hash
git config --global --add safe.directory $(pwd)

# Package release
#
# Use following for official releases
RELEASE="1"
# Use following for nightly builds
#RELEASE="0.$(date +%04Y%02m%02d%02H%02M).git$(git rev-parse --short ${GITHUB_SHA:-HEAD})"


# Set version and release
sed \
    -e "s|@VERSION@|${VERSION}|g" \
    -e "s|@RELEASE@|${RELEASE}|g" \
    < hirte.spec.in \
    > hirte.spec

# Prepare source archive
[[ -d rpmbuild/SOURCES ]] || mkdir -p rpmbuild/SOURCES
git archive --format=tar --prefix=hirte-$VERSION/ --add-file=hirte.spec HEAD \
    | gzip -9 > rpmbuild/SOURCES/hirte-$VERSION.tar.gz

# Build source package
rpmbuild \
    -D "_topdir rpmbuild" \
    -bs hirte.spec
