#!/usr/bin/bash
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

set -x

# Use PACKIT_COPR_PROJECT if set (for Packit CI), otherwise use default
COPR_REPO="${PACKIT_COPR_PROJECT:-@centos-automotive-sig/bluechi-snapshot}"

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
    local sshd_cfg="/etc/ssh/sshd_config.d/99-bluechi-tests.conf"
    echo "# BlueChi tests uses password authentication" > "$sshd_cfg"
    echo "PasswordAuthentication yes" >> "$sshd_cfg"
    echo "PermitRootLogin yes" >> "$sshd_cfg"

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

    # Enable COPR repository if requested (default: yes)
    if [ "${ENABLE_COPR:-yes}" == "yes" ]; then
        dnf install -y dnf-plugin-config-manager
        dnf copr enable -y "$COPR_REPO"
    fi

    # Build version suffix if BLUECHI_VERSION is specified
    # e.g. BLUECHI_VERSION="1.2.0"
    VERSION_SUFFIX=""
    if [ -n "$BLUECHI_VERSION" ]; then
        VERSION_SUFFIX="-${BLUECHI_VERSION}"
    fi

    dnf install \
        --nogpgcheck \
        --nodocs \
        bluechi-controller${VERSION_SUFFIX} \
        bluechi-controller-debuginfo${VERSION_SUFFIX} \
        bluechi-agent${VERSION_SUFFIX} \
        bluechi-agent-debuginfo${VERSION_SUFFIX} \
        bluechi-ctl${VERSION_SUFFIX} \
        bluechi-is-online${VERSION_SUFFIX} \
        bluechi-ctl-debuginfo${VERSION_SUFFIX} \
        bluechi-selinux${VERSION_SUFFIX} \
        python3-bluechi${VERSION_SUFFIX} \
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

    BUILD_ARGS=""

    # Pass COPR repository to container if COPR is enabled (default: yes)
    if [ "${ENABLE_COPR:-yes}" == "yes" ]; then
        BUILD_ARGS="--build-arg copr_repo=$COPR_REPO"
    fi

    # Pass BlueChi version to container if specified
    if [ -n "$BLUECHI_VERSION" ]; then
        BUILD_ARGS="$BUILD_ARGS --build-arg bluechi_version=$BLUECHI_VERSION"
    fi

    podman build \
        ${BUILD_ARGS} \
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
