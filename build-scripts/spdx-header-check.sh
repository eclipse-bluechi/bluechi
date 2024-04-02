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
# List of licenses which are OK when found in the project except examples
#
# Used for bluechi original code and in src/libbluechi/common/list.h
MAIN_LICENSES="LGPL-2.1-or-later"

#
# List of licenses which are OK when found in the project examples
#
# Used for examples in doc/api-examples and doc/bluechi-examples
EXAMPLES_LICENSES="MIT-0"

#
# List of files to check using the main project licenses
#
MAIN_FILES="$(find -type f \
    -regex ${CHECKED_FILES} \
    -not -path './builddir/*' \
    -not -path './subprojects/*' \
    -not -path './doc/*-examples/*' \
    -not -path './doc/diagrams.drawio.xml')"

#
# List of files to check using the examples licenses
#
EXAMPLES_FILES="$(find -type f \
    -regex ${CHECKED_FILES} \
    -path './doc/*-examples/*')"


#
# Iterate over files within a path and check the allowed licenses
#
# Parameters:
#    files    - a list of files to check the license
#    licences - a list of approved licenses
#
# Result:
#    cl_result - 0 if all files contain valid license, otherwise 1
#
check_licenses() {
    local files=$1
    local licenses=$2
    cl_result=0

    for f in ${files} ; do
        # scan for license only within the 1st 5 lines of each file
        license_found=$(head -n 5 ${f} | grep -Po '.*SPDX-License-Identifier\: \K\S+' || echo "-1")
        if [ "${license_found}" == "-1" ]; then
            cl_result=1
            echo "File '${f}' does not contain SPDX header!"
        else
            valid_license=1
            for l in ${licenses} ; do
                if [ "${license_found}" == "$l" ] ; then
                    valid_license=0
                fi
            done
            if [ "${valid_license}" == "1" ] ; then
                cl_result=1
                echo "File '${f}' contains unapproved license '${license_found}'"
            fi
        fi
    done
}


check_licenses "${MAIN_FILES}" "${MAIN_LICENSES}"
main_result=$cl_result
check_licenses "${EXAMPLES_FILES}" "${EXAMPLES_LICENSES}"
examples_result=$cl_result

exit $((${main_result} | ${examples_result}))
