
ALL_SRC_FILES = 'find . -name "*.[ch]" ! -path "./binchihua/src/ini/ini.[ch]" ! -path "./binchihua/src/common/hashmap/*" ! -path "./binchihua/test/**" ! -path "./builddir/**" -print0'

fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format -i --sort-includes {}

check-fmt:
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-format  --dry-run -Werror {}

lint: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} -- -D_GNU_SOURCE

lint-fix: 
	eval $(ALL_SRC_FILES) | xargs -0 -I {} clang-tidy --quiet {} --fix -- -D_GNU_SOURCE
