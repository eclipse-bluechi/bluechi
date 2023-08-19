#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -x

#TEST_NET_RANGE="10.10.0.0/24"
#BLUECHI_IMAGE_NAME="bluechi-image"
#CONTAINER_USED="integration-test-local"

echo "==== DOUG building image ===="
echo "CONTAINER_USER: $CONTAINER_USER"
echo "BLUECHI_IMAGE_NAME: $BLUECHI_IMAGE_NAME"
podman rmi $BLUECHI_IMAGE_NAME --force
podman build -f ./containers/$CONTAINER_USED -t $BLUECHI_IMAGE_NAME .
if [[ $? -ne 0 ]]; then
    echo "Failed to build bluechi image"
    exit 1
fi
echo "===== DOUG building image ===="

#modprobe iptable-nat

echo "====== DOUG setting netavark ======"
sudo apt install containers-common -y
sudo cp /usr/share/containers/containers.conf /etc/containers/containers.conf
#sudo sed -i '/#network_backend=*/c\network_backend="netavark"' /etc/containers/containers.conf
#sudo cat /etc/containers/containers.conf
sudo podman info --format {{.Host.NetworkBackend}}

echo "====== DOUG setting netavark ======"

    podman system reset --force
if ! podman network exists bluechi-test; then
    echo "========== DOUG Podman Network do not existe, creating..."
    net=$(podman network create --subnet $TEST_NET_RANGE bluechi-test)
    #net=$(podman network create bluechi-test)
    echo "$net"
    dpkg -l containernetworking-plugins podman podman-docker
    #network_backend = ""
    echo "========== DOUG Podman Network do not existe, creating..."
    echo "========== DOUG Podman Network do not existe, creating..."
fi

echo "========== DOUG podman network ls ========"
podman network ls
echo "========== DOUG podman network ls ========"

echo "========== DOUG inspect network =========="
podman network inspect bluechi-test
podman network inspect podman
echo "========== DOUG inspect network =========="

if [ "$(systemctl --user is-active podman.socket)" != "active" ]; then
    echo "Integration tests requires access to podman using user socket"
    systemctl --user enable podman.socket
    systemctl --user start podman.socket
fi
