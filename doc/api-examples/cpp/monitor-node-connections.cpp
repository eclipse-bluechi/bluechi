/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

class NodeListener {
    public:
        NodeListener(std::string name, sdbus::ObjectPath nodePath, sdbus::IConnection &connection) {
                this->nodeProxy = sdbus::createProxy(
                                connection, sdbus::ServiceName("org.eclipse.bluechi"), nodePath);

                this->nodeProxy->registerSignalHandler(
                                sdbus::InterfaceName("org.freedesktop.DBus.Properties"),
                                sdbus::SignalName("PropertiesChanged"),
                                [name](sdbus::Signal signal) -> int {
                                        std::string iface;
                                        signal >> iface;
                                        std::cout << name << "--" << iface << std::endl;

                                        try {
                                                std::vector< sdbus::DictEntry< std::string, sdbus::Variant > > items;
                                                signal >> items;

                                                for (auto &item : items) {
                                                        if (item.first == "Status") {
                                                                std::cout << "Node " << name << ": "
                                                                          << item.second.get< std::string >()
                                                                          << std::endl;
                                                        }
                                                }
                                        } catch (const sdbus::Error &e) {
                                                std::cout << "Failed to read signal: " << e.what()
                                                          << std::endl;
                                        }

                                        return 0;
                                });
        }

    private:
        std::unique_ptr< sdbus::IProxy > nodeProxy;
};


int main() {
        auto connection = sdbus::createSystemBusConnection();
        auto controllerProxy = sdbus::createProxy(
                        *connection,
                        sdbus::ServiceName("org.eclipse.bluechi"),
                        sdbus::ObjectPath("/org/eclipse/bluechi"));

        std::vector< sdbus::Struct< std::string, sdbus::ObjectPath, std::string, std::string > > nodes;
        controllerProxy->callMethod("ListNodes").onInterface("org.eclipse.bluechi.Controller").storeResultsTo(nodes);

        std::vector< NodeListener > listeners;
        for (auto &node : nodes) {
                std::string nodeName = node.get< 0 >();
                sdbus::ObjectPath nodePath = node.get< 1 >();

                listeners.push_back(NodeListener(nodeName, nodePath, *connection));
        }

        connection->enterEventLoop();
        return 0;
}
