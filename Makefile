all: orch orch-node orch-client

orch: orch.c orch.h types.h types.c
	gcc orch.c types.c -g -O1 -Wall -o orch `pkg-config --cflags --libs libsystemd`

orch-client: client.c orch.h
	gcc client.c -O1 -Wall -o orch-client `pkg-config --cflags --libs libsystemd`

orch-node: node.c orch.h  types.h types.c
	gcc node.c types.c -g -O1 -Wall -o orch-node `pkg-config --cflags --libs libsystemd`
