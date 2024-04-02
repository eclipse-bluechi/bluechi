#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

GCNO_DIR="/usr/share/bluechi-coverage"
GCDA_DIR="/var/tmp/bluechi-coverage"

BC_VRA="$(rpm -q --qf '%{VERSION}-%{RELEASE}.%{ARCH}' bluechi-agent 2>/dev/null)"
SRC_DIR="/usr/src/debug/bluechi-${BC_VRA}"

# Copy .gcno files to code coverage temporary directory
cp -r ${GCNO_DIR}/. ${GCDA_DIR}

# Copy source files from bluechi-debugsource package into GCDA_DIR, because unit test source files are not included
# in debugsource package, so we needed to add them to bluechi-coverage package
( cd $SRC_DIR ; cp -r src ${GCDA_DIR} )
