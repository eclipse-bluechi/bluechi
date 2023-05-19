.PHONY: installdeps srpm

installdeps:
	dnf -y install gcc git jq meson systemd-devel rpm-build

srpm: installdeps
	meson setup builddir
	ninja -C builddir srpm
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)
