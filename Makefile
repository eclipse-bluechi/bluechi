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

tests:
	sh run-tests.sh

build: build-client build-node build-orch

build-client:
	mkdir -p bin
	gcc src/client/client.c -g -Wall -o bin/client

build-node:
	mkdir -p bin
	gcc -D_GNU_SOURCE \
		src/node/node.c \
		src/node/opt.c \
		src/node/dbus.c \
		-g -Wall -o bin/node `pkg-config --cflags --libs libsystemd`

build-orch:
	mkdir -p bin
	gcc -D_GNU_SOURCE \
		src/orch/orch.c \
		src/orch/opt.c \
		src/orch/controller.c \
		-g -Wall -o bin/orch `pkg-config --cflags --libs libsystemd`
