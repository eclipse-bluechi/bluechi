#include "orch.h"
#include "types.h"

#include <time.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define DEBUG_DBUS_MESSAGES 0

typedef struct Orchestrator Orchestrator;
typedef struct Node Node;

struct Node {
        int ref_count;
        Orchestrator *orch;
        sd_bus *peer;
        sd_bus_slot *bus_slot;
        char *name;
        char *object_path;
        LIST_FIELDS(Node, nodes);
};

struct Orchestrator {
        Manager manager;
        LIST_HEAD(Node, nodes);
};

static Node *node_new(Orchestrator *orch) {
        Node *node = malloc0(sizeof (Node));
        if (node) {
                node->orch = orch;
                node->ref_count = 1;
                LIST_INIT(nodes, node);
        }
        return node;
}

static Node *node_ref(Node *node) {
        node->ref_count++;
        return node;
}

static void node_unref(Node *node) {
        node->ref_count--;

        if (node->ref_count == 0) {
                if (node->peer)
                        sd_bus_close_unref(node->peer);
                if (node->bus_slot)
                        sd_bus_slot_unref(node->bus_slot);
                if (node->name)
                        free(node->name);
                if (node->object_path)
                        free(node->object_path);
                free(node);
        }
}
_SD_DEFINE_POINTER_CLEANUP_FUNC(Node, node_unref);

static void orch_add_node(Orchestrator *orch, Node *node) {
        LIST_APPEND(nodes, orch->nodes, node_ref(node));
}

static void orch_remove_node(Orchestrator *orch, Node *node) {
        LIST_REMOVE(nodes, orch->nodes, node);
        node_unref(node);
}

static Node *orch_find_node(Orchestrator *orch, const char *name) {
        Node *node;

        LIST_FOREACH(nodes, node, orch->nodes) {
                if (node->name != NULL && strcmp (node->name, name) == 0)
                        return node;
        }

        return NULL;
}

static const sd_bus_vtable node_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_VTABLE_END
};

typedef struct {
        Job job;

        const char *target; /* owned by source_message */
        int outstanding;
}  IsolateAllJob;

static int job_isolate_all_cb (sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        Job *job = userdata;
        Manager *manager = job->manager;
        IsolateAllJob *isolate_all = (IsolateAllJob *)job;

        printf("job_isolate_all_cb %d, \n", isolate_all->outstanding);

        if (--isolate_all->outstanding == 0)
                manager_finish_job(manager, job);

        return 0;
}

static int job_isolate_all(Job *job, void *userdata) {
        IsolateAllJob *isolate_all = (IsolateAllJob *)job;
        Manager *manager = job->manager;
        Orchestrator *orch = (Orchestrator *)manager;
        Node *node;
        int r;

        printf ("Started job isolate all %p\n", isolate_all);

        LIST_FOREACH(nodes, node, orch->nodes) {
                _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
                _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
                _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

                r = sd_bus_message_new_method_call(node->peer, &m, NODE_BUS_NAME, NODE_PEER_OBJECT_PATH, NODE_PEER_IFACE, "Isolate");
                assert(r >= 0);

                r = sd_bus_message_append(m, "s", isolate_all->target);
                assert(r >= 0);

                /* TODO: NULL slot, never freed? */
                r = sd_bus_call_async(node->peer, NULL, m, job_isolate_all_cb, job, USEC_PER_SEC * 30);
                assert(r >= 0);

                isolate_all->outstanding++;
        }

        if (isolate_all->outstanding == 0)
                manager_finish_job(manager, job);

        return 0;
}

static int cancel_isolate_all(Job *job, void *userdata) {
        return 0;
}

static int method_orchestrator_isolate_all(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        Manager *manager = userdata;
        _cleanup_(job_unrefp) Job *job = NULL;
        IsolateAllJob *isolate_all;
        const char *target;
        int r;

        r = sd_bus_message_read(m, "s", &target);
        if (r < 0) {
                fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
                return r;
        }

        r = manager_queue_job(manager, JOB_ISOLATE_ALL, sizeof(IsolateAllJob),
                           job_isolate_all, cancel_isolate_all, NULL, NULL, &job);
        if (r < 0)
                return sd_bus_reply_method_errnof(m, -r, "Failed to create job: %m");

        isolate_all = (IsolateAllJob *)job;

        job->source_message = sd_bus_message_ref(m);
        isolate_all->target = target;

        return sd_bus_reply_method_return(m, "o", job->object_path);
}

static const sd_bus_vtable orchestrator_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("IsolateAll", "s", "o", method_orchestrator_isolate_all, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobNew",
                                 "uo",
                                 SD_BUS_PARAM(id)
                                 SD_BUS_PARAM(job),
                                 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobRemoved",
                                 "uos",
                                 SD_BUS_PARAM(id)
                                 SD_BUS_PARAM(job)
                                 SD_BUS_PARAM(result),
                                 0),
        SD_BUS_VTABLE_END
};

int create_master_socket(int port)
{
        int fd;
        struct sockaddr_in servaddr;

        fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to create socket: %m\n");
                return -errsv;
        }

        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                int errsv = errno;
                fprintf(stderr, "Failed to create socket: %m\n");
                return -errsv;
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if (bind(fd, &servaddr, sizeof(servaddr)) < 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to bind socket: %m\n");
                return -errsv;
        }

        if ((listen(fd, SOMAXCONN)) != 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to listed socket: %m\n");
                return -errsv;
        }

        return fd;
}

static int node_disconnected(sd_bus_message *message, void *userdata, sd_bus_error *error) {
        Node *node = userdata;

        if (node->name)
                printf("Node '%s' disconnected\n", node->name);
        else
                printf("Unregistered node disconnected\n");

        if (node->peer) {
                sd_bus_close_unref(node->peer);
                node->peer = NULL;
        }

        orch_remove_node(node->orch, node);

        return 0;
}

static int method_peer_orchestrator_register(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        Node *node = userdata;
        Orchestrator *orch = node->orch;
        Manager *manager = (Manager *)orch;
        Node *existing;
        int r;
        char *name;
        char description[100];

        /* Read the parameters */
        r = sd_bus_message_read(m, "s", &name);
        if (r < 0) {
                fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
                return r;
        }

        if (node->name != NULL)
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_ADDRESS_IN_USE, "Can't register twice");

        existing = orch_find_node(node->orch, name);
        if (existing != NULL)
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_ADDRESS_IN_USE, "Node name already registered");

        node->name = strdup(name);
        if (node->name == NULL)
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "No memory");

        r = asprintf(&node->object_path, "%s/%s", ORCHESTRATOR_NODES_OBJECT_PATH_PREFIX, name);
        if (r < 0)
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "No memory");

        strcpy(description, "node-");
        strncat(description, name, sizeof(description) - strlen(description) - 1);
        (void) sd_bus_set_description(node->peer, description);

        r = sd_bus_add_object_vtable(manager->bus,
                                     &node->bus_slot,
                                     node->object_path,
                                     ORCHESTRATOR_NODE_IFACE,
                                     node_vtable,
                                     node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        printf("Registered node on fd %d as '%s'\n", sd_bus_get_fd (node->peer), name);

        return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable peer_orchestrator_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Register", "s", "", method_peer_orchestrator_register, 0),
        SD_BUS_VTABLE_END
};

/* This is just some helper to make the peer connections look like a bus for e.g busctl */

static int method_peer_hello(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
        /* Reply with the response */
        return sd_bus_reply_method_return(m, "s", ":1.0");
}

static const sd_bus_vtable peer_bus_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Hello", "", "s", method_peer_hello, SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_VTABLE_END
};

static int
all_node_messages_handler (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
        Node *node = userdata;
        if (node->name)
                printf("Incomming message from node '%s' (fd %d): path: %s, iface: %s, member: %s, signature: '%s'\n",
                       node->name,
                       sd_bus_get_fd (node->peer),
                       sd_bus_message_get_path (m),
                       sd_bus_message_get_interface (m),
                       sd_bus_message_get_member (m),
                       sd_bus_message_get_signature (m, true));
        else
                printf("Incomming message from node fd %d: path: %s, iface: %s, member: %s, signature: '%s'\n",
                       sd_bus_get_fd (node->peer),
                       sd_bus_message_get_path (m),
                       sd_bus_message_get_interface (m),
                       sd_bus_message_get_member (m),
                       sd_bus_message_get_signature (m, true));
        return 0;
}

static int accept_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        Orchestrator *orch = userdata;
        Manager *manager = (Manager *)orch;
        _cleanup_fd_ int nfd = -1;
        _cleanup_(sd_bus_close_unrefp) sd_bus *bus = NULL;
        _cleanup_(node_unrefp) Node *node = NULL;
        sd_id128_t id;
        int r;

        nfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (nfd < 0) {
                if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                        return 0;
                else {
                        int errsv = errno;
                        fprintf(stderr, "Failed to accept: %m\n");
                        return -errsv;
                }
        }

        r = sd_bus_new(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to allocate new private connection bus: %m\n");
                return 0;
        }

        (void) sd_bus_set_description(bus, "node");
        r = sd_bus_set_trusted (bus, true); /* we trust everything from the node, there is only one peer anyway */
        if (r < 0) {
                fprintf(stderr, "Failed to trust node: %s\n", strerror(-r));
                return 0;
        }

        r = sd_bus_set_fd(bus, nfd, nfd);
        if (r < 0) {
                fprintf(stderr, "Failed to set fd on new connection bus: %s\n", strerror(-r));
                return 0;
        }

        nfd = -1;

        r = sd_id128_randomize(&id);
        assert (r >= 0);

        r = sd_bus_set_server(bus, 1, id);
        if (r < 0) {
                fprintf(stderr, "Failed to enable server support for new connection bus: %s\n", strerror(-r));
                return 0;
        }

        r = sd_bus_negotiate_creds(bus, 1,
                                   SD_BUS_CREDS_PID|SD_BUS_CREDS_UID|
                                   SD_BUS_CREDS_EUID|SD_BUS_CREDS_EFFECTIVE_CAPS|
                                   SD_BUS_CREDS_SELINUX_CONTEXT);
        if (r < 0) {
                fprintf(stderr, "Failed to enable credentials for new connection: %s\n", strerror(-r));
                return 0;
        }

        /* TODO: We don't want anonymous here really, but do it for now */
        r = sd_bus_set_anonymous(bus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set bus to anonymous: %s\n", strerror(-r));
                return 0;
        }

        r = sd_bus_set_sender(bus, ORCHESTRATOR_BUS_NAME);
        if (r < 0) {
                fprintf(stderr, "Failed to set direct connection sender: %s\n", strerror(-r));
                return 0;
        }

        r = sd_bus_start(bus);
        if (r < 0) {
                fprintf(stderr, "Failed to start new connection bus: %s", strerror(-r));
                return 0;
        }

        r = sd_bus_attach_event(bus, manager->event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach new connection bus to event loop: %s\n", strerror(-r));
                return 0;
        }

        node = node_new(orch);
        if (node == NULL) {
                fprintf(stderr, "Out of memory");
                return 0;
        }

        node->peer = steal_pointer(&bus);

        r = sd_bus_add_object_vtable(node->peer,
                                     NULL,
                                     "/org/freedesktop/DBus",
                                     "org.freedesktop.DBus",
                                     peer_bus_vtable,
                                     node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_bus_add_object_vtable(node->peer,
                                     NULL,
                                     ORCHESTRATOR_OBJECT_PATH,
                                     ORCHESTRATOR_PEER_IFACE,
                                     peer_orchestrator_vtable,
                                     node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_bus_match_signal_async(
                        node->peer,
                        NULL,
                        "org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local",
                        "Disconnected",
                        node_disconnected, NULL, node);
        if (r < 0) {
                fprintf(stderr, "Failed to request match for Disconnected message: %s\n", strerror(-r));
                return 0;
        }

        if (DEBUG_DBUS_MESSAGES)
                sd_bus_add_filter(node->peer, NULL, all_node_messages_handler, node);

        orch_add_node(node->orch, node);
        printf("Accepted new private connection on fd %d.\n", sd_bus_get_fd(node->peer));

        return 0;
}

static int
all_bus_messages_handler (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
        printf("Incomming message from bus: path: %s, iface: %s, member: %s, signature: '%s'\n",
               sd_bus_message_get_path (m),
               sd_bus_message_get_interface (m),
               sd_bus_message_get_member (m),
               sd_bus_message_get_signature (m, true));
        return 0;
}

int main(int argc, char *argv[]) {
        _cleanup_sd_event_ sd_event *event = NULL;
        _cleanup_sd_bus_slot_ sd_bus_slot *slot = NULL;
        _cleanup_sd_bus_ sd_bus *bus = NULL;
        _cleanup_fd_ int accept_fd = -1;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;
        int r;
        Orchestrator orchestrator = {};

        /* User bus for now */
        r = sd_bus_open_user(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        if (DEBUG_DBUS_MESSAGES)
                sd_bus_add_filter(bus, NULL, all_bus_messages_handler, NULL);

        orchestrator.manager.bus = bus;
        orchestrator.manager.job_path_prefix = ORCHESTRATOR_JOBS_OBJECT_PATH_PREFIX;
        orchestrator.manager.manager_path = ORCHESTRATOR_OBJECT_PATH;
        orchestrator.manager.manager_iface = ORCHESTRATOR_IFACE;

        r = sd_bus_add_object_vtable(bus,
                                     &slot,
                                     ORCHESTRATOR_OBJECT_PATH,
                                     ORCHESTRATOR_IFACE,
                                     orchestrator_vtable,
                                     &orchestrator);
        if (r < 0) {
                fprintf(stderr, "Failed to add vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_bus_request_name(bus, ORCHESTRATOR_BUS_NAME, 0);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        accept_fd = create_master_socket(1999);
        if (accept_fd < 0) {
                return EXIT_FAILURE;
        }

        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        orchestrator.manager.event = event;

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_event_add_io(event, &event_source, accept_fd, EPOLLIN,
                            accept_handler, &orchestrator);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        (void) sd_event_source_set_description(event_source, "master-socket");

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Event loop failed: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
