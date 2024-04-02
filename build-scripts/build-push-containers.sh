#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# The 1st parameter is the image name
IMAGE="$1"

# The 2nd parameter is optional and it specifies the container architecture. If omitted, all archs will be built.
ARCHITECTURES="${2:-amd64 arm64}"
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_DIR=$SCRIPT_DIR"/../"
CONTAINER_FILE_DIR=$SCRIPT_DIR"/../containers"

function push(){
    buildah manifest push --all $IMAGE "docker://quay.io/bluechi/$IMAGE"
}

function build(){
    # remove old image, ignore result
    buildah manifest rm $IMAGE &> /dev/null || true

    buildah manifest create $IMAGE

    for arch in $ARCHITECTURES; do
        buildah bud --tag "quay.io/bluechi/$IMAGE" \
            --manifest $IMAGE \
            --arch ${arch} \
            -f ${CONTAINER_FILE_DIR}/${IMAGE} \
            ${ROOT_DIR}
    done
}

[ -z ${IMAGE} ] && echo "Requires image name. Either 'build-base' or 'integration-test-base'." && exit 1

echo "Building containers and manifest for '${IMAGE}'"
echo ""
build
if [ "${PUSH_MANIFEST}" == "yes" ]; then
    push
fi
