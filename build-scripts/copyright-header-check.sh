#!/usr/bin/bash -e
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# Checked files regex
#
CHECKED_FILES=".*\(\.c\|\.h\|\.go\|\.py\|\.rs\|\.sh\|\.xml\|meson\.build\)"

#
# Regex to match the required text of the copyright header
#
COPYRIGHT_REGEX=".*Contributors to the Eclipse BlueChi project.*"

#
# List of files to check using the copyright header
#
FILE_LIST="$(find -type f \
        -regex ${CHECKED_FILES} \
        -not -path './builddir/*' \
        -not -path './subprojects/*' \
        -not -path './doc/diagrams.drawio.xml' \
        -not -path './selinux/build-selinux.sh' \
        -not -path './selinu/meson.build' \
    )"


hc_result=0

for f in ${FILE_LIST} ; do
    # scan for copyright only within the 1st 5 lines of each file
    copyright_found=$(head -n 5 ${f} | sed -nr 's/.*Copyright (.*)/\1/p')
    if [ -z "${copyright_found}" ]; then
        hc_result=1
        echo "File '${f}' does not contain copyright header!"
    elif ! [[ ${copyright_found} =~ ${COPYRIGHT_TEXT} ]] ; then
        hc_result=1
        echo "File '${f}' contains invalid copyright header: '${copyright_found}'!"
    fi
done

exit ${hc_result}
