.PHONY: installdeps srpm

installdeps:
	dnf -y install git

srpm: installdeps
	./rpm/build-srpm.sh
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)
