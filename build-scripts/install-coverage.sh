#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

# This script should be executed only from meson as a part of install flow!
#
# Install all created `*.gcno` files so they could be packaged into bluechi-coverage RPM.
COV_DIR="${MESON_INSTALL_DESTDIR_PREFIX}/share/bluechi-coverage"
mkdir -p "${COV_DIR}"
find "${MESON_BUILD_ROOT}" -name '*.gcno' -exec cp "{}" "${COV_DIR}" \;
