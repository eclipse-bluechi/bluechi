#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

SCRIPT_DIR=$( realpath "$0"  )
SCRIPT_DIR=$(dirname "$SCRIPT_DIR")/
CONTAINER_FILE_DIR=$SCRIPT_DIR"../tests/containers/"

IMAGE=""

function build_and_push(){
    buildah manifest rm $IMAGE &> /dev/null
    buildah manifest create $IMAGE

    buildah bud --tag "quay.io/bluechi/$IMAGE" \
        --manifest $IMAGE \
        --arch amd64 ${CONTAINER_FILE_DIR}$IMAGE

    buildah bud --tag "quay.io/bluechi/$IMAGE" \
        --manifest $IMAGE \
        --arch arm64 ${CONTAINER_FILE_DIR}$IMAGE

    if [ "${PUSH_MANIFEST}" == "yes" ]; then
        buildah manifest push --all $IMAGE "docker://quay.io/bluechi/$IMAGE"
    fi
}

function build_base(){
    IMAGE="build-base"
    build_and_push
}

function integration_test_base(){
    IMAGE="integration-test-base"
    build_and_push
}

echo "Building containers and manifest for $1"
echo "" 
$1
