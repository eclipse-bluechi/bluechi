TARGETS?=hirte
MODULES?=${TARGETS:=.pp.bz2}
SHAREDIR?=/usr/share
HIRTEDIR=/var/lib/hirte

all: ${TARGETS:=.pp.bz2}

%.pp.bz2: %.pp
	@echo Compressing $^ -\> $@
	bzip2 -f -9 $^

%.pp: %.te
	make -f ${SHAREDIR}/selinux/devel/Makefile $@

clean:
	rm -f *~  *.tc *.pp *.pp.bz2
	rm -rf tmp *.tar.gz

man: install-policy
	sepolicy manpage --path . --domain ${TARGETS}_t
	sepolicy manpage --path . --domain ${TARGETS}_agent_t

install-policy: all
	semodule -i ${TARGETS}.pp.bz2

install: all install-policy man
	install -D -m 644 ${TARGETS}.pp.bz2 ${DESTDIR}${SHAREDIR}/selinux/packages/hirte.pp.bz2
	install -D -m 644 hirte.if ${DESTDIR}${SHAREDIR}/selinux/devel/include/services/hirte.if
	install -D -m 644 hirte_selinux.8 ${DESTDIR}${SHAREDIR}/man/man8/hirte_selinux.8
	install -D -m 644 hirte_agent_selinux.8 ${DESTDIR}${SHAREDIR}/man/man8/hirte_agent_selinux.8
	semanage port -a -t hirte_port_t -p udp 842 || true
	semanage port -a -t hirte_port_t -p tcp 842 || true
