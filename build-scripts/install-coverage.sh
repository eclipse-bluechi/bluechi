#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

# This script should be executed only from meson as a part of install flow!
#
# Install all created `*.gcno` files so they could be packaged into bluechi-coverage RPM.

for f in $MESON_BUILD_ROOT/src/*; do
    if [ -d "$f" ]; then
        COV_DIR="${MESON_INSTALL_DESTDIR_PREFIX}/share/bluechi-coverage"
        COV_DIR="${COV_DIR}/${f##*/}"
        mkdir -p "${COV_DIR}"
        find "${f}" -name '*.gcno' -exec cp "{}" "${COV_DIR}" \;
    fi
done
