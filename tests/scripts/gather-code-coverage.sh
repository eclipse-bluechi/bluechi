#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later


# First parameter is the name of the generated info file
INFO_FILE=${1:-coverage.info}

source $(dirname "$(readlink -f "$0")")/setup-src-dir-for-coverage.sh

# Move each .gcda file into the respective project directory containing the .gcno
for file in ${GCDA_DIR}/*.gcda ; do
    # Remove encoded directories up to src
    tmp_f="src${file##*src}"
    # Convert encoded directory to proper path
    tmp_f="${tmp_f//\#/\/}"
    mv -v $file ${GCDA_DIR}/${tmp_f}
done

# Generate info file
geninfo ${GCDA_DIR} -b ${GCDA_DIR}/src -o ${INFO_FILE}
