#include "orch.h"

static const char *waiting_for_job = NULL;
static sd_bus_message *waiting_for_job_result = NULL;

static sd_bus_message *wait_for_job(sd_bus *bus, const char *object_path) {
	int r;

	assert(waiting_for_job == NULL);
	waiting_for_job = object_path;

	for (;;) {
		/* Process requests */
		r = sd_bus_process(bus, NULL);
		if (r < 0) {
			fprintf(stderr, "Failed to process bus: %s\n",
				strerror(-r));
			return NULL;
		}

		/* Did we get the result? */
		if (waiting_for_job_result != NULL)
			return steal_pointer(&waiting_for_job_result);

		if (r >
		    0) /* we processed a request, try to process another one, right-away */
			continue;

		/* Wait for the next request to process */
		r = sd_bus_wait(bus, (uint64_t)-1);
		if (r < 0) {
			fprintf(stderr, "Failed to wait on bus: %s\n",
				strerror(-r));
			return NULL;
		}
	}
}

static int match_job_removed(sd_bus_message *m, void *userdata,
			     sd_bus_error *error) {
	const char *job_path;
	const char *result;
	uint32_t id;
	int r;

	if (waiting_for_job == NULL)
		return 0;

	r = sd_bus_message_read(m, "uos", &id, &job_path, &result);
	if (r < 0) {
		fprintf(stderr, "Can't parse job result\n");
		return 0;
	}

	(void)sd_bus_message_rewind(m, true);

	if (strcmp(waiting_for_job, job_path) == 0) {
		waiting_for_job_result = sd_bus_message_ref(m);
		return 1;
	}

	return 0;
}

int isolate_all(int argc, char *argv[], sd_bus *bus) {
	_cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
	_cleanup_sd_bus_message_ sd_bus_message *m = NULL;
	_cleanup_sd_bus_message_ sd_bus_message *job_result = NULL;
	int r;
	const char *target;
	const char *job_path;
	const char *result;
	uint32_t id;

	if (argc < 1) {
		fprintf(stderr, "No target given\n");
		return -EINVAL;
	}
	target = argv[0];

	/* Issue the method call and store the respons message in m */
	r = sd_bus_call_method(bus, ORCHESTRATOR_BUS_NAME,
			       ORCHESTRATOR_OBJECT_PATH, ORCHESTRATOR_IFACE,
			       "IsolateAll", &error, &m, "s", target);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s\n",
			error.message);
		return r;
	}

	/* Parse the response message */
	r = sd_bus_message_read(m, "o", &job_path);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n",
			strerror(-r));
		return r;
	}

	printf("Started IsolateAll operation\n");

	job_result = wait_for_job(bus, job_path);
	if (job_result == NULL) {
		return -EIO;
	}

	r = sd_bus_message_read(job_result, "uos", &id, &job_path, &result);
	if (r < 0) {
		fprintf(stderr, "Can't parse job result\n");
		return 0;
	}

	printf("IsolateAll result: %s\n", result);

	return 0;
}

int main(int argc, char *argv[]) {
	_cleanup_sd_bus_ sd_bus *bus = NULL;
	const char *command;
	int r;

	if (argc < 2) {
		fprintf(stderr, "No command given\n");
		return EXIT_FAILURE;
	}
	command = argv[1];
	argv = &argv[2];
	argc -= 2;

	/* Connect to the system bus (user for now) */
	r = sd_bus_open_user(&bus);
	if (r < 0) {
		fprintf(stderr, "Failed to connect to system bus: %s\n",
			strerror(-r));
		return EXIT_FAILURE;
	}

	r = sd_bus_match_signal(bus, NULL, ORCHESTRATOR_BUS_NAME,
				ORCHESTRATOR_OBJECT_PATH, ORCHESTRATOR_IFACE,
				"JobRemoved", match_job_removed, NULL);
	if (r < 0)
		return r;

	if (strcmp("isolate-all", command) == 0) {
		r = isolate_all(argc, argv, bus);
	} else {
		fprintf(stderr, "Unknown command: %s\n", command);
		return EXIT_FAILURE;
	}

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
