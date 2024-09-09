.PHONY: build fmt check-fmt lint lint-c-fix codespell rpm srpm

DESTDIR ?=
BUILDDIR=builddir

CODESPELL_PARAMS=\
	-S Makefile,imgtype,copy,AUTHORS,bin,.git,CHANGELOG.md,changelog.txt,.cirrus.yml,"*.xz,*.gz,*.tar,*.tgz,*ico,*.png,*.1,*.5,*.orig,*.rej,*.xml,*xsl",build.ninja,intro-targets.json,./tests/tests/tier0/proxy-service-fails-on-typo-in-file/systemd/simple.service,tags,./builddir,./subprojects,\
	-L keypair,flate,uint,iff,od,ERRO,crate,te \
	--ignore-regex=".*codespell-ignore$$"

build:
	meson setup $(BUILDDIR)
	meson compile -C $(BUILDDIR)

test:
	meson setup $(BUILDDIR)
	meson configure -Db_coverage=true $(BUILDDIR)
	meson compile -C $(BUILDDIR)
	meson test -C $(BUILDDIR)
	ninja coverage-html -C $(BUILDDIR)

test-with-valgrind: build
	meson test --wrap='valgrind --leak-check=full --error-exitcode=1 --track-origins=yes' -C $(BUILDDIR)

install: build
	meson install -C $(BUILDDIR) --destdir "$(DESTDIR)"

fmt: build
	ninja -C $(BUILDDIR) clang-format

check-fmt: build
	ninja -C $(BUILDDIR) clang-format-check

lint-c: build
	ninja -C $(BUILDDIR) clang-tidy

lint-c-fix: build
	ninja -C $(BUILDDIR) clang-tidy-fix

lint-markdown:
	markdownlint-cli2 | echo "markdownlint-cli2 not found, skipping markdown lint"

lint: lint-c lint-markdown

codespell:
	codespell  $(CODESPELL_PARAMS) -w

check-codespell:
	codespell  $(CODESPELL_PARAMS)

clean:
	find . -name \*~ -delete
	find . -name \*# -delete
	meson setup --wipe $(BUILDDIR)

rpm: srpm
	@build-scripts/build-rpm.sh

srpm:
	@rpmbuild || (echo "For building RPM package, rpmbuild command is required. To install use: dnf install rpm-build"; exit 1)
	@build-scripts/build-srpm.sh

distclean: clean
	rm -rf $(BUILDDIR)
