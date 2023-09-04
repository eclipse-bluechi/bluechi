#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

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
    < bluechi.spec.in \
    > bluechi.spec

# Prepare source archive
[[ -d rpmbuild/SOURCES ]] || mkdir -p rpmbuild/SOURCES

git archive --format=tar --prefix=bluechi-$VERSION/ --add-file=bluechi.spec -o bluechi-$VERSION.tar HEAD
git submodule foreach --recursive \
    "git archive --prefix=bluechi-$VERSION/\$path/ --output=\$sha1.tar HEAD && \
     tar --concatenate --file=$(pwd)/bluechi-$VERSION.tar \$sha1.tar && rm \$sha1.tar"
gzip bluechi-$VERSION.tar
mv bluechi-$VERSION.tar.gz rpmbuild/SOURCES/

# Build source package
rpmbuild \
    -D "_topdir rpmbuild" \
    -bs bluechi.spec
