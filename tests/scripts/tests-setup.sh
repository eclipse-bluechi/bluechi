#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -x

PODMAN_BACKEND_CONFIG_FILE="/etc/containers/containers.conf.d/50-network-backend.conf"
if [ "$GITHUB_ACTIONS" == "true" ]; then
    expected_backend="netavark"
    podman_network_backend=$(sudo podman info --format "{{.Host.NetworkBackend}}")

    if [ "${podman_network_backend}" != "${expected_backend}" ]; then
        echo "Setting Host Network Backend as netavark..."
        sudo mkdir -p /etc/containers/containers.conf.d
        sudo echo "[network]" > "${PODMAN_BACKEND_CONFIG_FILE}"
        sudo echo "network_backend = \"netavark\"" >> "${PODMAN_BACKEND_CONFIG_FILE}"
        sudo podman system reset --force
    fi
fi

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
