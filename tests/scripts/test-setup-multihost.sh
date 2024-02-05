#!/usr/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later

function managed_install_dependencies(){
    dnf upgrade --refresh --nodocs -y
    dnf install --nodocs epel-release -y
    dnf install --nodocs \
            lcov \
            lsof \
            policycoreutils-python-utils \
            python3-dasbus \
            selinux-policy \
            systemd \
            systemd-devel \
            valgrind \
        -y
    dnf -y clean all
}

function managed_install_rpms(){
    if ["${BLUECHI_RPM_USED}" == "local"]; then
        RPM_DIR="${TMT_TREE}/${BLUECHI_RPM_DIR}"
        dnf install --repo bluechi-rpms \
            --repofrompath bluechi-rpms,file://"${RPM_DIR}" \
            --nogpgcheck \
            --nodocs \
            bluechi-controller \
            bluechi-controller-debuginfo \
            bluechi-agent \
            bluechi-agent-debuginfo \
            bluechi-ctl \
            bluechi-ctl-debuginfo \
            bluechi-selinux \
            python3-bluechi \
            -y
    elif ["${BLUECHI_RPM_USED}" == "snapshot"]; then
        dnf install -y dnf-plugin-config-manager
        dnf copr enable -y "${BLUECHI_RPM_COPR}"
        dnf install \
            --nogpgcheck \
            --nodocs \
            bluechi-controller \
            bluechi-controller-debuginfo \
            bluechi-agent \
            bluechi-agent-debuginfo \
            bluechi-ctl \
            bluechi-ctl-debuginfo \
            bluechi-selinux \
            python3-bluechi \
            -y
    else
        echo "Invalid option for 'BLUECHI_RPM_USED'. Has to be one of [local, snapshot]." && exit 1
    fi
}

function setup_managed(){
    if [ "${INSTALL_MANAGED_DEPS}" == "yes" ]; then
        managed_install_dependencies
    fi
    managed_install_rpms
}

function manager_install_dependencies(){
    dnf install \
        python3-dasbus \
        python3-pytest \
        python3-pytest-timeout \
        python3-pyyaml \
        python3-paramiko \
        -y
}

function setup_manager(){
    if [ "${INSTALL_MANAGER_DEPS}" == "yes" ]; then
        manager_install_dependencies
    fi
}

[ -z $1 ] && echo "Missing parameter. Use either 'setup_manager' or 'setup_managed'." && exit 1

echo "Calling '$1'"
echo ""
$1
