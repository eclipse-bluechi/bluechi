#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

SCRIPT_DIR=$( realpath "$0"  )
SCRIPT_DIR=$(dirname "$SCRIPT_DIR")/
CONTAINER_FILE_DIR=$SCRIPT_DIR"../tests/containers/"

function push(){
    buildah manifest push --all $IMAGE "docker://quay.io/bluechi/$IMAGE"
}

function build(){
    buildah manifest rm $IMAGE &> /dev/null
    buildah manifest create $IMAGE

    buildah bud --tag "quay.io/bluechi/$IMAGE" \
        --manifest $IMAGE \
        --arch amd64 ${CONTAINER_FILE_DIR}$IMAGE

    buildah bud --tag "quay.io/bluechi/$IMAGE" \
        --manifest $IMAGE \
        --arch arm64 ${CONTAINER_FILE_DIR}$IMAGE
}

[ -z $1 ] && echo "Requires image name. Either 'build-base' or 'integration-test-base'." && exit 1

echo "Building containers and manifest for '$1'"
echo ""
IMAGE="$1"
build
if [ "${PUSH_MANIFEST}" == "yes" ]; then
    push
fi
