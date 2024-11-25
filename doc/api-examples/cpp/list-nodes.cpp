/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

int main() {
        auto connection = sdbus::createSystemBusConnection();
        auto controllerProxy = sdbus::createProxy(
                        *connection,
                        sdbus::ServiceName("org.eclipse.bluechi"),
                        sdbus::ObjectPath("/org/eclipse/bluechi"));

        std::vector< sdbus::Struct< std::string, sdbus::ObjectPath, std::string, std::string > > nodes;
        sdbus::ObjectPath nodePath;
        controllerProxy->callMethod("ListNodes").onInterface("org.eclipse.bluechi.Controller").storeResultsTo(nodes);

        for (auto &node : nodes) {
                std::cout << "Node: " << node.get< 0 >() << ", Status: " << node.get< 2 >() << std::endl;
        }

        return 0;
}
