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