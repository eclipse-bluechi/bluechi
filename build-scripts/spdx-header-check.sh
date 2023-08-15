#!/usr/bin/bash -e
# SPDX-License-Identifier: LGPL-2.1-or-later

# Checked files regex
CHECKED_FILES=".*\(\.c\|\.h\|\.py\|\.sh\|\.xml\|meson\.build\)"

#
# List of licenses which are OK when found in a source code
#
APPROVED_LICENSES=""
# Used for bluechi original code and in src/libbluechi/common/list.h
APPROVED_LICENSES="${APPROVED_LICENSES} LGPL-2.1-or-later"
# Used for examples in doc/api-examples and doc/bluechi-examples
APPROVED_LICENSES="${APPROVED_LICENSES} CC0-1.0"

result=0
found_files="
    $(find -type f -regex ${CHECKED_FILES} \
        -not -path './builddir/*' \
        -not -path './subprojects/*')
"

for f in ${found_files} ; do
    # scan for license only within the 1st 5 lines of each file
    license_found=$(head -n 5 ${f} | grep -Po '.*SPDX-License-Identifier\: \K\S+' || echo "-1")
    if [ "${license_found}" == "-1" ]; then
        result=1
        echo "File '${f}' does not contain SPDX header!"
    else
        valid_license=1
        for l in ${APPROVED_LICENSES} ; do
            if [ "${license_found}" == "$l" ] ; then
                valid_license=0
            fi
        done
        if [ "${valid_license}" == "1" ] ; then
            result=1
            echo "File '${f}' contains unapproved license '${license_found}'"
        fi
    fi
done

exit ${result}
