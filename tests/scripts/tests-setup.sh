#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -x

podman build -f ./containers/$CONTAINER_USED -t $BLUECHI_IMAGE_NAME .
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
