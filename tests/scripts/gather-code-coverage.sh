#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later


# First parameter is the name of the generated info file
INFO_FILE=${1:-coverage.info}
BLUECHI_VERSION=${2:-unknown}

# Find the bluechi debug directory dynamically to extract version
BLUECHI_DEBUG_DIR=$(find /usr/src/debug -maxdepth 1 -type d -name 'bluechi-*' | head -1)
if [ -z "$BLUECHI_DEBUG_DIR" ]; then
    echo "Error: No bluechi-* directory found in /usr/src/debug"
    exit 1
fi

# Construct the BUILD directory path using the extracted version
BLUECHI_BUILD_DIR="/github/home/rpmbuild/BUILD/bluechi-${BLUECHI_VERSION}"

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
# Use literal paths to avoid variable expansion issues
geninfo ${GCDA_DIR} \
  --substitute "s|${BLUECHI_BUILD_DIR}/|/var/tmp/bluechi-coverage/|g" \
  --ignore-errors source \
  --output-file ${INFO_FILE}