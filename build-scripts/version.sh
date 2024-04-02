#!/bin/bash -e
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Current version of BlueChi
VERSION=0.8.0
# Specify if build is a official release or a snapshot build
IS_RELEASE=false
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

    if [ $IS_RELEASE = false ]; then
        # Used for nightly builds
        if [ -f "${RELEASE_FILE}" ]; then
            RELEASE="$(cat ${RELEASE_FILE})"
        else
            RELEASE="0.$(date +%04Y%02m%02d%02H%02M).git$(git rev-parse --short ${GITHUB_SHA:-HEAD})"
            echo ${RELEASE} > ${RELEASE_FILE}
        fi
    fi
    echo $RELEASE
}

function clean(){
    rm -f "${RELEASE_FILE}"
}

[ -z $1 ] && short || $1
