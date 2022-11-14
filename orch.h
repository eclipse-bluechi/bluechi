#pragma once
#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#define ELEMENTSOF(x)                                                        \
	(__builtin_choose_expr(!__builtin_types_compatible_p(typeof(x),      \
							     typeof(&*(x))), \
			       sizeof(x) / sizeof((x)[0]), NULL))

/* The head of the linked list. Use this in the structure that shall
 * contain the head of the linked list */
#define LIST_HEAD(t, name) t *name

/* The pointers in the linked list's items. Use this in the item structure */
#define LIST_FIELDS(t, name) t *name##_next, *name##_prev

/* Initialize the list's head */
#define LIST_HEAD_INIT(head)   \
	do {                   \
		(head) = NULL; \
	} while (false)

/* Initialize a list item */
#define LIST_INIT(name, item)                                   \
	do {                                                    \
		typeof(*(item)) *_item = (item);                \
		assert(_item);                                  \
		_item->name##_prev = _item->name##_next = NULL; \
	} while (false)

/* Prepend an item to the list */
#define LIST_PREPEND(name, head, item)                              \
	do {                                                        \
		typeof(*(head)) **_head = &(head), *_item = (item); \
		assert(_item);                                      \
		if ((_item->name##_next = *_head))                  \
			_item->name##_next->name##_prev = _item;    \
		_item->name##_prev = NULL;                          \
		*_head = _item;                                     \
	} while (false)

/* Append an item to the list */
#define LIST_APPEND(name, head, item)                          \
	do {                                                   \
		typeof(*(head)) **_hhead = &(head), *_tail;    \
		LIST_FIND_TAIL(name, *_hhead, _tail);          \
		LIST_INSERT_AFTER(name, *_hhead, _tail, item); \
	} while (false)

/* Remove an item from the list */
#define LIST_REMOVE(name, head, item)                                         \
	do {                                                                  \
		typeof(*(head)) **_head = &(head), *_item = (item);           \
		assert(_item);                                                \
		if (_item->name##_next)                                       \
			_item->name##_next->name##_prev = _item->name##_prev; \
		if (_item->name##_prev)                                       \
			_item->name##_prev->name##_next = _item->name##_next; \
		else {                                                        \
			assert(*_head == _item);                              \
			*_head = _item->name##_next;                          \
		}                                                             \
		_item->name##_next = _item->name##_prev = NULL;               \
	} while (false)

/* Find the head of the list */
#define LIST_FIND_HEAD(name, item, head)                    \
	do {                                                \
		typeof(*(item)) *_item = (item);            \
		if (!_item)                                 \
			(head) = NULL;                      \
		else {                                      \
			while (_item->name##_prev)          \
				_item = _item->name##_prev; \
			(head) = _item;                     \
		}                                           \
	} while (false)

/* Find the tail of the list */
#define LIST_FIND_TAIL(name, item, tail)                    \
	do {                                                \
		typeof(*(item)) *_item = (item);            \
		if (!_item)                                 \
			(tail) = NULL;                      \
		else {                                      \
			while (_item->name##_next)          \
				_item = _item->name##_next; \
			(tail) = _item;                     \
		}                                           \
	} while (false)

/* Insert an item after another one (a = where, b = what) */
#define LIST_INSERT_AFTER(name, head, a, b)                              \
	do {                                                             \
		typeof(*(head)) **_head = &(head), *_a = (a), *_b = (b); \
		assert(_b);                                              \
		if (!_a) {                                               \
			if ((_b->name##_next = *_head))                  \
				_b->name##_next->name##_prev = _b;       \
			_b->name##_prev = NULL;                          \
			*_head = _b;                                     \
		} else {                                                 \
			if ((_b->name##_next = _a->name##_next))         \
				_b->name##_next->name##_prev = _b;       \
			_b->name##_prev = _a;                            \
			_a->name##_next = _b;                            \
		}                                                        \
	} while (false)

/* Insert an item before another one (a = where, b = what) */
#define LIST_INSERT_BEFORE(name, head, a, b)                             \
	do {                                                             \
		typeof(*(head)) **_head = &(head), *_a = (a), *_b = (b); \
		assert(_b);                                              \
		if (!_a) {                                               \
			if (!*_head) {                                   \
				_b->name##_next = NULL;                  \
				_b->name##_prev = NULL;                  \
				*_head = _b;                             \
			} else {                                         \
				typeof(*(head)) *_tail = (head);         \
				while (_tail->name##_next)               \
					_tail = _tail->name##_next;      \
				_b->name##_next = NULL;                  \
				_b->name##_prev = _tail;                 \
				_tail->name##_next = _b;                 \
			}                                                \
		} else {                                                 \
			if ((_b->name##_prev = _a->name##_prev))         \
				_b->name##_prev->name##_next = _b;       \
			else                                             \
				*_head = _b;                             \
			_b->name##_next = _a;                            \
			_a->name##_prev = _b;                            \
		}                                                        \
	} while (false)

#define LIST_JUST_US(name, item) (!(item)->name##_prev && !(item)->name##_next)

#define LIST_FOREACH(name, i, head) \
	for ((i) = (head); (i); (i) = (i)->name##_next)

#define LIST_FOREACH_SAFE(name, i, n, head) \
	for ((i) = (head); (i) && (((n) = (i)->name##_next), 1); (i) = (n))

#define LIST_FOREACH_BEFORE(name, i, p) \
	for ((i) = (p)->name##_prev; (i); (i) = (i)->name##_prev)

#define LIST_FOREACH_AFTER(name, i, p) \
	for ((i) = (p)->name##_next; (i); (i) = (i)->name##_next)

/* Iterate through all the members of the list p is included in, but skip over p */
#define LIST_FOREACH_OTHERS(name, i, p)                              \
	for (({                                                      \
		     (i) = (p);                                      \
		     while ((i) && (i)->name##_prev)                 \
			     (i) = (i)->name##_prev;                 \
		     if ((i) == (p))                                 \
			     (i) = (p)->name##_next;                 \
	     });                                                     \
	     (i); (i) = (i)->name##_next == (p) ? (p)->name##_next : \
							(i)->name##_next)

/* Loop starting from p->next until p->prev.
   p can be adjusted meanwhile. */
#define LIST_LOOP_BUT_ONE(name, i, head, p)                                  \
	for ((i) = (p)->name##_next ? (p)->name##_next : (head); (i) != (p); \
	     (i) = (i)->name##_next ? (i)->name##_next : (head))

#define LIST_IS_EMPTY(head) (!(head))

/* Join two lists tail to head: a->b, c->d to a->b->c->d and de-initialise second list */
#define LIST_JOIN(name, a, b)                              \
	do {                                               \
		assert(b);                                 \
		if (!(a))                                  \
			(a) = (b);                         \
		else {                                     \
			typeof(*(a)) *_head = (b), *_tail; \
			LIST_FIND_TAIL(name, (a), _tail);  \
			_tail->name##_next = _head;        \
			_head->name##_prev = _tail;        \
		}                                          \
		(b) = NULL;                                \
	} while (false)

static inline void *steal_pointer(void *ptr) {
	void **ptrp = (void **)ptr;
	void *p = *ptrp;
	*ptrp = NULL;
	return p;
}

static inline void closep(int *fd) {
	if (*fd >= 0) {
		close(*fd);
	}
}

#define malloc0(n) (calloc(1, (n) ?: 1))

static inline void freep(void *p) {
	free(*(void **)p);
}

#define _cleanup_(x) __attribute__((__cleanup__(x)))

#define _cleanup_free_ _cleanup_(freep)
#define _cleanup_fd_ _cleanup_(closep)
#define _cleanup_sd_event_ _cleanup_(sd_event_unrefp)
#define _cleanup_sd_event_source_ _cleanup_(sd_event_source_unrefp)
#define _cleanup_sd_bus_ _cleanup_(sd_bus_unrefp)
#define _cleanup_sd_bus_slot_ _cleanup_(sd_bus_slot_unrefp)
#define _cleanup_sd_bus_message_ _cleanup_(sd_bus_message_unrefp)

#define USEC_PER_SEC ((uint64_t)1000000ULL)
#define NSEC_PER_USEC ((uint64_t)1000ULL)

#define BUS_DEFINE_PROPERTY_GET2(function, bus_type, data_type, get1, get2) \
	int function(sd_bus *bus, const char *path, const char *interface,  \
		     const char *property, sd_bus_message *reply,           \
		     void *userdata, sd_bus_error *error) {                 \
		data_type *data = userdata;                                 \
                                                                            \
		assert(bus);                                                \
		assert(reply);                                              \
		assert(data);                                               \
                                                                            \
		return sd_bus_message_append(reply, bus_type,               \
					     get2(get1(data)));             \
	}

#define ident(x) (x)
#define BUS_DEFINE_PROPERTY_GET(function, bus_type, data_type, get1) \
	BUS_DEFINE_PROPERTY_GET2(function, bus_type, data_type, get1, ident)

#define ref(x) (*(x))
#define BUS_DEFINE_PROPERTY_GET_REF(function, bus_type, data_type, get) \
	BUS_DEFINE_PROPERTY_GET2(function, bus_type, data_type, ref, get)

#define BUS_DEFINE_PROPERTY_GET_ENUM(function, name, type) \
	BUS_DEFINE_PROPERTY_GET_REF(function, "s", type, name##_to_string)

static inline const char *enum_to_string(int i, const char *const *table,
					 int table_size) {
	if (i < 0 || i >= table_size)
		return NULL;
	return table[i];
}
#define ENUM_TO_STRING(i, table) \
	(enum_to_string((int)(i), table, ELEMENTSOF(table)))

#define ORCHESTRATOR_BUS_NAME "com.redhat.Orchestrator"
#define ORCHESTRATOR_OBJECT_PATH "/com/redhat/Orchestrator"
#define ORCHESTRATOR_NODES_OBJECT_PATH_PREFIX "/com/redhat/Orchestrator/node"
#define ORCHESTRATOR_JOBS_OBJECT_PATH_PREFIX "/com/redhat/Orchestrator/job"
#define ORCHESTRATOR_IFACE "com.redhat.Orchestrator"
#define ORCHESTRATOR_NODE_IFACE "com.redhat.Orchestrator.Node"
#define ORCHESTRATOR_PEER_IFACE "com.redhat.Orchestrator.Peer"

#define JOB_IFACE "com.redhat.Orchestrator.Job"

#define NODE_BUS_NAME "com.redhat.Orchestrator.Node"
#define NODE_PEER_OBJECT_PATH "/com/redhat/Orchestrator/Node"
#define NODE_PEER_JOBS_OBJECT_PATH_PREFIX "/com/redhat/Orchestrator/Node/job"
#define NODE_IFACE "com.redhat.Orchestrator.Node"
#define NODE_PEER_IFACE "com.redhat.Orchestrator.Node.Peer"

#define DEFAULT_DBUS_TIMEOUT (USEC_PER_SEC * 30)

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"
