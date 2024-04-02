#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Clean previously generated release cache
$(dirname "$(readlink -f "$0")")/version.sh clean

# Create source archive
source $(dirname "$(readlink -f "$0")")/create-archive.sh

mv bluechi-$VERSION.tar.gz rpmbuild/SOURCES/

# Build source package
rpmbuild \
    -D "_topdir rpmbuild" \
    -bs bluechi.spec
