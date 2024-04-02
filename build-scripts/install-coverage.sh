#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# This script should be executed only from meson as a part of install flow!
#
COVERAGE_ROOT="${MESON_INSTALL_DESTDIR_PREFIX}/share/bluechi-coverage"

# Install all created `*.gcno` files so they could be packaged into bluechi-coverage RPM.
mkdir -p ${COVERAGE_ROOT}
( cd $MESON_BUILD_ROOT ; find . -name "*.gcno" -exec cp -v --parents {} ${COVERAGE_ROOT} \; )
