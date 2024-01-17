#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

GCNO_DIR="/usr/share/bluechi-coverage"
GCDA_DIR="/var/tmp/bluechi-coverage"

# First parameter is the name of the generated info file
INFO_FILE=${1:-coverage.info}

BC_VRA="$(rpm -q --qf '%{VERSION}-%{RELEASE}.%{ARCH}' bluechi-agent 2>/dev/null)"
SRC_DIR="/usr/src/debug/bluechi-${BC_VRA}/src"

# Copy .gcno files to code coverage temporary directory
cp -r ${GCNO_DIR}/. ${GCDA_DIR}

# move each .gcda file into the respective project directory containing the .gcno
for file in ${GCDA_DIR}/*.gcda ; do
    # project directory, e.g. libbluechi, is at position 3 (hence -f-3)
    project_dir=$(echo $file | rev | cut -d'#' -f-3 | rev | cut -d'#' -f-1)
    filename=$(echo $file | rev | cut -d'#' -f-1 | rev)
    mv $file ${GCDA_DIR}/$project_dir/$filename
done

# Generate info file
geninfo ${GCDA_DIR} -b ${SRC_DIR} -o ${INFO_FILE}
