#!/bin/bash -xe

source $(dirname "$(readlink -f "$0")")/build-srpm.sh

# Install build dependencies
dnf builddep -y rpmbuild/SRPMS/*src.rpm

# Build binary package
rpmbuild \
    --define "_topmdir rpmbuild" \
    --define "_rpmdir rpmbuild" \
    --rebuild rpmbuild/SRPMS/*src.rpm

# Move RPMs to exported artifacts
[[ -d $ARTIFACTS_DIR ]] || mkdir -p $ARTIFACTS_DIR
find rpmbuild -iname \*rpm | xargs mv -t $ARTIFACTS_DIR
