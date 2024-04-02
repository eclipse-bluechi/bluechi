#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This script should not be executed manually, it's executed automatically during RPM build with code coverage support
#

GCDA_DIR="/var/tmp/bluechi-coverage"
MESON_BUILD_ROOT="$1"
DATA_DIR="$2"
TMP_INFO_FILE="${MESON_BUILD_ROOT}/meson-logs/coverage.info"
INFO_FILE="${DATA_DIR}/bluechi-coverage/unit-test-results/coverage.info"

mkdir -p ${GCDA_DIR}

# Copy all sources directory where GCDA files are generated
( cd ${MESON_BUILD_ROOT}/.. ; find . \( -name "*.c" -o -name "*.h" \) -exec cp -v --parents {} ${GCDA_DIR} \; )

# Copy all sources and gcno files from meson build directory into directory where GCDA files are generated
( cd ${MESON_BUILD_ROOT} ; find . -name "*.gcno" -exec cp -v --parents {} ${GCDA_DIR} \; )

# Move each .gcda file into the respective project directory containing the .gcno
for file in ${GCDA_DIR}/*.gcda ; do
    # Remove encoded directories up to src
    tmp_f="src${file##*src}"
    # Convert encoded directory to proper path
    tmp_f="${tmp_f//\#/\/}"
    mv -v $file ${GCDA_DIR}/${tmp_f}
done

# Generate info file for unit tests code coverage
geninfo ${GCDA_DIR} -b ${GCDA_DIR}/src -o ${TMP_INFO_FILE}

# Remove tests executable files from generate code coverage info, because we want to measure only non-testing source
# files, and code coverage .info file into bluchi-coverage package, so it can be merged with integration test results
# later
lcov --remove ${TMP_INFO_FILE} -o ${INFO_FILE} '*/src/*/test/*'

# Make path to source code local to current directory, because source codes will be on a different place during
# the merge with integration test results
sed -i 's/SF:.*src/SF:\/var\/tmp\/bluechi-coverage\/src/' ${INFO_FILE}
