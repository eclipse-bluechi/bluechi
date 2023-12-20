#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

GCNO_DIR="/usr/share/bluechi-coverage"
GCDA_DIR="/var/tmp/bluechi-coverage"

# First parameter is the name of the generated info file
INFO_FILE=${1:-coverage.info}

BC_VRA="$(rpm -q --qf '%{VERSION}-%{RELEASE}.%{ARCH}' bluechi-agent 2>/dev/null)"
SRC_DIR="/usr/src/debug/bluechi-${BC_VRA}/src"

# Remove path prefix from GCDA files
(cd ${GCDA_DIR} && for file in *.gcda ; do mv ${file} ${file##*#} ; done)

# Copy .gcno files to code coverage temporary directory
cp -r ${GCNO_DIR}/. ${GCDA_DIR}

# Generate info file
geninfo ${GCDA_DIR} -b ${SRC_DIR} -o ${INFO_FILE}
