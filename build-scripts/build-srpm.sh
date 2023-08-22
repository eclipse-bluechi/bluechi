#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

source $(dirname "$(readlink -f "$0")")/create-archive.sh

mv bluechi-$VERSION.tar.gz rpmbuild/SOURCES/

# Build source package
rpmbuild \
    -D "_topdir rpmbuild" \
    -bs bluechi.spec
