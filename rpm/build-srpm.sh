#!/bin/bash -xe
# SPDX-License-Identifier: GPL-2.0-or-later

# Prepare source archive
[[ -d rpmbuild/SOURCES ]] || mkdir -p rpmbuild/SOURCES
git archive --format=tar --add-file=$1 HEAD | gzip -9 > rpmbuild/SOURCES/hirte.tar.gz

# Build source package
rpmbuild \
    -D "_topdir rpmbuild" \
    -bs $1
