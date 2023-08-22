#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

source $(dirname "$(readlink -f "$0")")/create-spec.sh

# Prepare source archive
[[ -d rpmbuild/SOURCES ]] || mkdir -p rpmbuild/SOURCES

git archive --format=tar --prefix=bluechi-$VERSION/ --add-file=bluechi.spec -o bluechi-$VERSION.tar HEAD
git submodule foreach --recursive \
    "git archive --prefix=bluechi-$VERSION/\$path/ --output=\$sha1.tar HEAD && \
     tar --concatenate --file=$(pwd)/bluechi-$VERSION.tar \$sha1.tar && rm \$sha1.tar"
gzip bluechi-$VERSION.tar
