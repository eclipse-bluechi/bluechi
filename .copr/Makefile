.PHONY: installdeps srpm

installdeps:
	dnf -y install gcc gcc-c++ git jq libselinux-devel meson systemd-devel rpm-build

srpm: installdeps
	git config --global --add safe.directory "$(shell pwd)"
	./build-scripts/build-srpm.sh
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)
