#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

set -x

# Install required dependencies only when asked
if [ "$INSTALL_DEPS" == "yes" ]; then
    dnf install \
        podman \
        python3-dasbus \
        python3-podman \
        python3-pytest \
        python3-pytest-timeout \
        python3-yaml \
        python3-paramiko \
        -y
    # Mitigate https://github.com/containers/podman-py/issues/350
    dnf install python3-rich -y
fi

BUILD_ARG=""

# By default we want to use bluechi-snapshot repo, but when building from packit testing farm we need to pass custom
# copr repo to the container (packit uses temporary COPR repos for unmerged PRs)
COPR_REPO="@centos-automotive-sig/bluechi-snapshot"
if [ "$PACKIT_COPR_PROJECT" != "" ]; then
    COPR_REPO="$PACKIT_COPR_PROJECT"
fi
BUILD_ARG="--build-arg copr_repo=$COPR_REPO"

podman build \
    ${BUILD_ARG} \
    -f ./containers/$CONTAINER_USED \
    -t $BLUECHI_IMAGE_NAME \
    .
if [[ $? -ne 0 ]]; then
    exit 1
fi

if [ "$(systemctl --user is-active podman.socket)" != "active" ]; then
    echo "Integration tests requires access to podman using user socket"
    systemctl --user enable podman.socket
    systemctl --user start podman.socket
fi
