.PHONY: installdeps srpm

installdeps:
	dnf -y install gcc gcc-c++ git jq meson systemd-devel rpm-build

srpm: installdeps
	./build-scripts/build-srpm.sh
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)
