#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

set -x

if [ "$CONTAINER_USED" = "integration-test-snapshot" ]; then
   export ARCH="--arch=$(uname -m)"
fi

BUILD_ARG=""
if [ "$WITH_COVERAGE" == "1" ]; then
    BUILD_ARG="--build-arg with_coverage=1"
fi

podman build $ARCH $BUILD_ARG -f ./containers/$CONTAINER_USED -t $BLUECHI_IMAGE_NAME .

if [[ $? -ne 0 ]]; then
    exit 1
fi

podman network exists bluechi-test
if [[ $? -ne 0 ]]; then
    podman network create --subnet $TEST_NET_RANGE bluechi-test
fi

if [ "$(systemctl --user is-active podman.socket)" != "active" ]; then
    echo "Integration tests requires access to podman using user socket"
    systemctl --user enable podman.socket
    systemctl --user start podman.socket
fi
