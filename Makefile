fmt:
	for file in ./src/**/*.[ch] ; do \
		clang-format -i --sort-includes $${file} ; \
	done

check-fmt:
	for file in ./src/**/*.[ch] ; do \
		clang-format --dry-run -Werror $${file} ; \
		if [ $$? -ne 0 ] ; then \
			exit 1 ; \
		fi ; \
	done

lint: 
	find . -name "*.[ch]" ! -path "./src/ini/*" ! -path "./test/**" -print0 | xargs -0 -I {} clang-tidy --quiet {} -- -D_GNU_SOURCE

lint-fix: 
	find . -name "*.[ch]" ! -path "./src/ini/*" ! -path "./test/**" -print0 | xargs -0 -I {} clang-tidy --quiet {} --fix -- -D_GNU_SOURCE
