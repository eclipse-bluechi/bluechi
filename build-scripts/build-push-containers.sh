#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# The 1st parameter is the image name
IMAGE="$1"

# The 2nd parameter is the centos stream version
CENTOS_STREAM_VERSION="$2"

# The 3rd parameter is optional and it specifies the container architecture. If omitted, all archs will be built.
ARCHITECTURES="${3:-amd64 arm64}"
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_DIR=$SCRIPT_DIR"/../"
CONTAINER_FILE_DIR=$SCRIPT_DIR"/../containers"

IMAGE_NAME="${IMAGE}:${CENTOS_STREAM_VERSION}"
QUAY_NAME="quay.io/bluechi/${IMAGE_NAME}"

function push(){
    buildah manifest push --all "${IMAGE_NAME}" "docker://${QUAY_NAME}"
}

function build(){
    # remove old image, ignore result
    buildah manifest rm "${IMAGE_NAME}" &> /dev/null || true

    buildah manifest create "${IMAGE_NAME}"

    for arch in $ARCHITECTURES; do
        buildah bud --tag "${QUAY_NAME}" \
            --manifest "${IMAGE_NAME}" \
            --arch "${arch}" \
            --build-arg "centos_stream_version=${CENTOS_STREAM_VERSION}" \
            -f "${CONTAINER_FILE_DIR}/${IMAGE}" \
            "${ROOT_DIR}"
    done
}

[ -z ${IMAGE} ] && echo "Requires image name. Either 'build-base' or 'integration-test-base'." && exit 1

echo "Building containers and manifest for '${IMAGE}'"
echo ""
build
if [ "${PUSH_MANIFEST}" == "yes" ]; then
    push
fi
