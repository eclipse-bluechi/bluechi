#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later


# First parameter is the name of the generated info file
INFO_FILE=${1:-coverage.info}
GCDA_DIR=${2:-/var/tmp/bluechi-coverage}

# Move each .gcda file into the respective project directory containing the .gcno
for file in ${GCDA_DIR}/*.gcda ; do
    # Remove encoded directories up to src
    tmp_f="src${file##*src}"
    # Convert encoded directory to proper path
    tmp_f="${tmp_f//\#/\/}"
    mv -v $file ${GCDA_DIR}/${tmp_f}
done

BLUECHI_BUILD_DIR=".*/rpmbuild/BUILD/bluechi[^/]*"
# Generate info file
# Substitute build directory paths with coverage directory paths using regex pattern
geninfo ${GCDA_DIR} \
  --substitute "s|${BLUECHI_BUILD_DIR}/|/var/tmp/bluechi-coverage/|g" \
  --ignore-errors source \
  --output-file ${INFO_FILE}
