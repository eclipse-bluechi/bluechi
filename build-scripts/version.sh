#!/bin/bash -e
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Current version of BlueChi
VERSION=0.10.2
# Specify if build is a official release or a snapshot build
IS_RELEASE="0"
# Used for official releases. Increment if necessary
RELEASE="1"
# Used to cache generated snapshot release for further usage
RELEASE_FILE=$(dirname "$(readlink -f "$0")")/RELEASE

function short(){
    echo ${VERSION}
}

function long(){
    echo "$(short)-$(release)"
}

function release(){
    # Package release
    if [ -f "${RELEASE_FILE}" ]; then
        # Read existing release from release file to keep the same release during the whole build
        RELEASE="$(cat ${RELEASE_FILE})"
    else
        # Release file doesn't exist, populate it with a release
        if [ $IS_RELEASE == "0" ]; then
            RELEASE="0.$(date +%04Y%02m%02d%02H%02M).git$(git rev-parse --short ${GITHUB_SHA:-HEAD})"
        fi
        echo ${RELEASE} > ${RELEASE_FILE}
    fi
    echo $RELEASE
}

function clean(){
    rm -f "${RELEASE_FILE}"
}

[ -z $1 ] && short || $1
