#include "orch.h"
#include "types.h"

typedef struct Node Node;

struct Node {
        Manager manager;
        sd_bus *local_bus;
};

#define DEBUG_DBUS_MESSAGES 0

typedef struct {
        Job job;
        const char *target; /* owned by source_message */
}  IsolateJob;

static int job_isolate(Job *job, void *userdata) {
        Manager *manager = job->manager;
        IsolateJob *isolate = (IsolateJob *)job;

        printf ("Running Isolate %s\n", isolate->target);

        manager_finish_job(manager, job);

        return 0;
}

static int method_node_isolate(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        Node *node = userdata;
        Manager *manager = (Manager *)node;
        const char *target;
        _cleanup_(job_unrefp) Job *job = NULL;
        IsolateJob *isolate;
        int r;

        /* TODO: Make this a job */
        r = sd_bus_message_read(m, "s", &target);
        if (r < 0) {
                fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
                return r;
        }

        printf("Got Isolate '%s'\n", target);

        r = manager_queue_job(manager, NODE_JOB_ISOLATE, sizeof(IsolateJob),
                              job_isolate, NULL, NULL,
                              NULL, &job);
        if (r < 0)
                return sd_bus_reply_method_errnof(m, -r, "Failed to create job: %m");

        isolate = (IsolateJob *)job;

        job->source_message = sd_bus_message_ref(m);
        isolate->target = target;

        return sd_bus_reply_method_return(m, "");
}


static const sd_bus_vtable node_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Isolate", "s", "", method_node_isolate, 0),
        SD_BUS_VTABLE_END
};

static int
all_messages_handler (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
        printf("Incomming message from orchestrator: path: %s, iface: %s, member: %s, signature: '%s'\n",
               sd_bus_message_get_path (m),
               sd_bus_message_get_interface (m),
               sd_bus_message_get_member (m),
               sd_bus_message_get_signature (m, true));
        return 0;
}

int main(int argc, char *argv[]) {
        _cleanup_sd_event_ sd_event *event = NULL;
        _cleanup_sd_bus_ sd_bus *bus = NULL;
        _cleanup_sd_bus_ sd_bus *orch = NULL;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
        int r;
        _cleanup_free_ char *dbus_addr = NULL;
        int orchestrator_port = 1999;
        const char *orchestrator_address;
        const char *node_name;
        Node node = { };

        if (argc < 2) {
                fprintf(stderr, "No orchestrator address given\n");
                return EXIT_FAILURE;
        }

        orchestrator_address = argv[1];

        if (argc < 3) {
                fprintf(stderr, "No node name given\n");
                return EXIT_FAILURE;
        }

        node_name = argv[2];

        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        node.manager.event = event;

        /* Connect to system bus (for talking to systemd) */

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        node.local_bus = bus;

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach new connection bus to event loop: %s\n", strerror(-r));
                return 0;
        }

        /* Connect to orchestrator */

        r = sd_bus_new(&orch);
        if (r < 0) {
                fprintf(stderr, "Failed to create bus: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        node.manager.bus = orch;
        node.manager.job_path_prefix = NODE_PEER_JOBS_OBJECT_PATH_PREFIX;
        node.manager.manager_path = NODE_PEER_OBJECT_PATH;
        node.manager.manager_iface = NODE_IFACE;

        (void) sd_bus_set_description(bus, "orchestrator");
        r = sd_bus_set_trusted (orch, true); /* we trust everything from the orchestrator, there is only one peer anyway */
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", orchestrator_address, orchestrator_port);
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return EXIT_FAILURE;
        }

        r = sd_bus_set_address(orch, dbus_addr);
        if (r < 0) {
                fprintf(stderr, "Failed to set address: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        /* Connect to the orchastrator */
        r = sd_bus_start(orch);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to orchestrator: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        if (DEBUG_DBUS_MESSAGES)
                sd_bus_add_filter(orch, NULL, all_messages_handler, NULL);

        r = sd_bus_add_object_vtable(orch,
                                     NULL,
                                     NODE_PEER_OBJECT_PATH,
                                     NODE_PEER_IFACE,
                                     node_vtable,
                                     &node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        /* Register with orchestrator */
        r = sd_bus_call_method(orch,
                               ORCHESTRATOR_BUS_NAME,
                               ORCHESTRATOR_OBJECT_PATH,
                               ORCHESTRATOR_PEER_IFACE,
                               "Register",
                               &error,
                               &m,
                               "s",
                               node_name,
                               "replace");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                sd_bus_error_free(&error);
                return EXIT_FAILURE;
        }

        r = sd_bus_message_read(m, "");
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        printf("Registered as '%s'\n", node_name);

        r = sd_bus_attach_event(orch, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Event loop failed: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
