/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>


static int on_properties_changed(sdbus::Signal signal) {
        std::string iface;
        signal >> iface;

        try {
                std::vector< sdbus::DictEntry< std::string, sdbus::Variant > > items;
                signal >> items;

                for (auto &item : items) {
                        if (item.first == "Status") {
                                std::cout << "System status: " << item.second.get< std::string >()
                                          << std::endl;
                        }
                }
        } catch (const sdbus::Error &e) {
                std::cout << "Failed to read signal: " << e.what() << std::endl;
        }

        return 0;
}

int main() {
        auto connection = sdbus::createSystemBusConnection();
        auto controllerProxy = sdbus::createProxy(
                        *connection,
                        sdbus::ServiceName("org.eclipse.bluechi"),
                        sdbus::ObjectPath("/org/eclipse/bluechi"));

        controllerProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.freedesktop.DBus.Properties"),
                        sdbus::SignalName("PropertiesChanged"),
                        on_properties_changed);

        connection->enterEventLoop();
        return 0;
}
