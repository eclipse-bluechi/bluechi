#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

source $(dirname "$(readlink -f "$0")")/build-srpm.sh

# Install build dependencies
if [ "${SKIP_BUILDDEP}" != "yes" ]; then
    dnf builddep -y rpmbuild/SRPMS/*src.rpm
fi

# Build binary package
rpmbuild \
    --define "_topmdir rpmbuild" \
    --define "_rpmdir rpmbuild" \
    --define "with_coverage ${WITH_COVERAGE:=0}"\
    --define "with_python ${WITH_PYTHON:=1}" \
    --rebuild rpmbuild/SRPMS/*src.rpm

# Move RPMs to exported artifacts
ARTIFACTS_DIR="${ARTIFACTS_DIR:=$(pwd)/artifacts/rpms-$(date +%04Y%02m%02d%02H%02M)}"
[[ -d $ARTIFACTS_DIR ]] || mkdir -p $ARTIFACTS_DIR
find rpmbuild -iname \*rpm | xargs mv -t $ARTIFACTS_DIR
