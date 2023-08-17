#!/bin/bash -xe
# SPDX-License-Identifier: GPL-2.0-or-later

SCRIPT_DIR=$( realpath "$0"  )
SCRIPT_DIR=$(dirname "$SCRIPT_DIR")/
CONTAINER_FILE_DIR=$SCRIPT_DIR"../tests/containers/"

PLATFORMS=linux/amd64,linux/arm64

function build_base(){
    podman build --no-cache \
        --platform ${PLATFORMS} \
        -f ${CONTAINER_FILE_DIR}build-base \
        --manifest quay.io/bluechi/build-base .
}

function integration_test_base(){
    podman build --no-cache \
        --platform ${PLATFORMS} \
        -f ${CONTAINER_FILE_DIR}integration-test-base \
        --manifest quay.io/bluechi/integration-test-base .
}

echo "Building containers and manifest for $1"
echo ""
$1
