#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

set -x

COPR_REPO="@centos-automotive-sig/bluechi-snapshot"

function install_worker_deps(){
    dnf install \
        podman \
        python3-dasbus \
        python3-paramiko \
        python3-podman \
        python3-pytest \
        python3-pyyaml \
        -y
}

function setup_worker_ssh(){
    dnf install openssh-server passwd -y

    # root access is needed in order to access BlueChi's API
    echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config
    echo "PermitRootLogin yes" >> /etc/ssh/sshd_config

    systemctl restart sshd

    if [ -z "${SSH_USER}" ]; then
        SSH_USER="root"
    fi
    if [ -z "${SSH_PASSWORD}" ]; then
        SSH_PASSWORD="root"
    fi
    echo "$SSH_PASSWORD" | passwd "$SSH_USER" --stdin
}

function setup_worker(){
    if [ "$INSTALL_WORKER_DEPS" == "yes" ]; then
        dnf upgrade --refresh --nodocs -y
        dnf install --nodocs epel-release -y
        dnf install --nodocs \
                lcov \
                lsof \
                policycoreutils-python-utils \
                procps-ng \
                python3-dasbus \
                selinux-policy \
                systemd \
                systemd-devel \
                valgrind \
            -y
    fi

    dnf install -y dnf-plugin-config-manager
    dnf copr enable -y @centos-automotive-sig/bluechi-snapshot
    dnf install \
        --nogpgcheck \
        --nodocs \
        bluechi-controller \
        bluechi-controller-debuginfo \
        bluechi-agent \
        bluechi-agent-debuginfo \
        bluechi-ctl \
        bluechi-is-online \
        bluechi-ctl-debuginfo \
        bluechi-selinux \
        python3-bluechi \
        -y
    dnf -y clean all

    if [ "$SETUP_SSH" == "yes" ]; then
        setup_worker_ssh
    fi
}

function setup_executor(){
    # Install required dependencies only when asked
    if [ "$INSTALL_EXECUTOR_DEPS" == "yes" ]; then
        install_worker_deps
    fi
}



function setup_multihost_test(){
    [ -z $1 ] && echo "Missing option for multihost test setup. Pass in either 'setup_executor' or 'setup_worker'" && exit 1
    $1
}

function setup_container_test(){
    # Install required dependencies only when asked
    if [ "$INSTALL_DEPS" == "yes" ]; then
        install_worker_deps
    fi

    BUILD_ARG=""

    # By default we want to use bluechi-snapshot repo, but when building from packit testing farm we need to pass custom
    # copr repo to the container (packit uses temporary COPR repos for unmerged PRs)
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
        echo "podman build failed"
        exit 123
    fi

    if [ "$(systemctl --user is-active podman.socket)" != "active" ]; then
        echo "Integration tests requires access to podman using user socket"
        systemctl --user enable podman.socket
        systemctl --user start podman.socket
    fi
}

[ -z $1 ] && echo "Requires type of test. Either 'setup_container_test' or 'setup_multihost_test'" && exit 1
if [ "$1" == "setup_multihost_test" ]; then
    $1 $2
    exit 0
fi

$1
