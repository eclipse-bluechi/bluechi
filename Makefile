
ALL_SRC_FILES = 'find . -name "*.[ch]" ! -path "./src/libhirte/ini/ini.[ch]" ! -path "./src/libhirte/hashmap/*" ! -path "./src/*/test/**" ! -path "./builddir/**" -print0'

fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format -i --sort-includes {}

check-fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format  --dry-run -Werror {}

lint: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} -- -I src/ -D_GNU_SOURCE

lint-fix: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} --fix -- -I src/ -D_GNU_SOURCE

codespell:
	codespell -S Makefile,imgtype,copy,AUTHORS,bin,.git,CHANGELOG.md,changelog.txt,.cirrus.yml,"*.xz,*.gz,*.tar,*.tgz,*ico,*.png,*.1,*.5,*.orig,*.rej,*.xml" -L keypair,flate,uint,iff,od,ERRO -w
