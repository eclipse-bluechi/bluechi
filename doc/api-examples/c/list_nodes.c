// SPDX-License-Identifier: LGPL-2.1-or-later
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

void handle_structure(DBusMessageIter *structIter) {
    int typeInStruct;
    while ((typeInStruct = dbus_message_iter_get_arg_type(structIter)) != DBUS_TYPE_INVALID) {
        if (typeInStruct == DBUS_TYPE_STRING) {
            char *value;
            dbus_message_iter_get_basic(structIter, &value);
            printf("%s\n", value);
        } else if (typeInStruct == DBUS_TYPE_OBJECT_PATH) {
            char *path;
            dbus_message_iter_get_basic(structIter, &path);
            printf("Path: %s\n", path);
        }
        dbus_message_iter_next(structIter);
    }
    printf("\n");
}

int main() {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg;
    DBusMessageIter args;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }

    msg = dbus_message_new_method_call(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        "org.eclipse.bluechi.Manager",
        "ListNodes");

    if (msg == NULL) {
        fprintf(stderr, "Message Null\n");
        return 1;
    }

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn,
        msg,
        -1,
        &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Send Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }

    if (!reply) {
        fprintf(stderr, "Reply Null\n");
        return 1;
    }

    if (!dbus_message_iter_init(reply, &args)) {
        fprintf(stderr, "Message has no arguments!\n");
    } else {
        int argType = dbus_message_iter_get_arg_type(&args);

        if (argType == DBUS_TYPE_ARRAY) {
            DBusMessageIter subArgs;
            dbus_message_iter_recurse(&args, &subArgs);

            while ((argType = dbus_message_iter_get_arg_type(&subArgs)) != DBUS_TYPE_INVALID) {
                if (argType == DBUS_TYPE_STRUCT) {
                    DBusMessageIter structArgs;
                    dbus_message_iter_recurse(&subArgs, &structArgs);
                    handle_structure(&structArgs);
                } else {
                    printf("Unexpected item: %d\n", argType);
                }

                dbus_message_iter_next(&subArgs);
            }
        } else {
            fprintf(stderr, "Argument is not an array!\n");
        }
    }

    dbus_message_unref(msg);
    dbus_message_unref(reply);
    dbus_connection_unref(conn);

    return 0;
}

