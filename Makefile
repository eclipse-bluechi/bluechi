
ALL_SRC_FILES = 'find . -name "*.[ch]" ! -path "./hirte/src/ini/ini.[ch]" ! -path "./hirte/src/common/hashmap/*" ! -path "./hirte/test/**" ! -path "./libhirte/test/**" ! -path "./builddir/**" -print0'

fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format -i --sort-includes {}

check-fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format  --dry-run -Werror {}

lint: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} -- -D_GNU_SOURCE

lint-fix: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} --fix -- -D_GNU_SOURCE
