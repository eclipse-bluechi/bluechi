#include "../common/dbus.h"
#include "controller.h"

#include <stdio.h>
#include <stdlib.h>

int getArgs(int argc, char *argv[], int *ctrlPort) {
	if (argc < 2) {
		fprintf(stderr, "No controller port given\n");
		return -1;
	}
	*ctrlPort = atoi(argv[1]);

	return 0;
}

int main(int argc, char *argv[]) {
	fprintf(stdout, "Hello from orchestrator!\n");

	int port;
	if (getArgs(argc, argv, &port) == -1) {
		return EXIT_FAILURE;
	}

	int r;
	_cleanup_sd_event_ sd_event *event = NULL;
	r = sd_event_default(&event);
	if (r < 0) {
		fprintf(stderr, "Failed to create event: %s\n", strerror(-r));
		return EXIT_FAILURE;
	}

	_cleanup_sd_event_source_ sd_event_source *event_source = NULL;
	r = controller_setup(port, event, event_source);
	if (r < 0) {
		fprintf(stderr, "Failed to setup controller");
		return EXIT_FAILURE;
	}

	r = sd_event_loop(event);
	if (r < 0) {
		fprintf(stderr, "Event loop failed: %s\n", strerror(-r));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}