.PHONY: installdeps srpm

installdeps:
	dnf -y install git

srpm: installdeps
	./build-scripts/build-srpm.sh
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)
