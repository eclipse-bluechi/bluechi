#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

COV_DIR="${MESON_INSTALL_DESTDIR_PREFIX}/share/bluechi-coverage"
mkdir -p "${COV_DIR}"
find "${MESON_BUILD_ROOT}" -name '*.gcno' -exec cp "{}" "${COV_DIR}" \;

