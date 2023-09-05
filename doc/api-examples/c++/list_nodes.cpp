// SPDX-License-Identifier: LGPL-2.1-or-later
#include <dbus/dbus.h>
#include <iostream>

void handle_structure(DBusMessageIter *structIter) {
    int typeInStruct;
    while ((typeInStruct = dbus_message_iter_get_arg_type(
                structIter)) != DBUS_TYPE_INVALID) {
        if (typeInStruct == DBUS_TYPE_STRING) {
            char *value;
            dbus_message_iter_get_basic(structIter, &value);
            std::cout << "" << value << std::endl;
        } else if (typeInStruct == DBUS_TYPE_OBJECT_PATH) {
            char *path;
            dbus_message_iter_get_basic(structIter, &path);
            std::cout << "Path: " << path << std::endl;
        }
        dbus_message_iter_next(structIter);
    }
    std::cout << "" << std::endl;
}


int main() {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg;
    DBusMessageIter args;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "Connection Error: " << err.message << std::endl;
        dbus_error_free(&err);
        return 1;
    }

    msg = dbus_message_new_method_call(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        "org.eclipse.bluechi.Manager",
        "ListNodes");

    if (msg == nullptr) {
        std::cerr << "Message Null" << std::endl;
        return 1;
    }

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn,
        msg,
        -1,
        &err);

    if (dbus_error_is_set(&err)) {
        std::cerr << "Send Error: " << err.message << std::endl;
        dbus_error_free(&err);
        return 1;
    }

    if (!reply) {
        std::cerr << "Reply Null" << std::endl;
        return 1;
    }

    if (!dbus_message_iter_init(reply, &args)) {
        std::cerr << "Message has no arguments!" << std::endl;
    } else {
        int argType = dbus_message_iter_get_arg_type(&args);

        if (argType == DBUS_TYPE_ARRAY) {
            DBusMessageIter subArgs;
            dbus_message_iter_recurse(&args, &subArgs);

            while ((argType = dbus_message_iter_get_arg_type(
                    &subArgs)) != DBUS_TYPE_INVALID) {
                if (argType == DBUS_TYPE_STRUCT) {
                    DBusMessageIter structArgs;
                    dbus_message_iter_recurse(&subArgs, &structArgs);
                    handle_structure(&structArgs);
                } else {
                    std::cout << "Unexpected item: " << argType << std::endl;
                }

                dbus_message_iter_next(&subArgs);
            }
        } else {
            std::cerr << "Argument is not an array!" << std::endl;
        }
    }

    dbus_message_unref(msg);
    dbus_message_unref(reply);
    dbus_connection_unref(conn);

    return 0;
}
