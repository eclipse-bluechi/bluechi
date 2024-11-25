/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

int main(int argc, char *argv[]) {
        if (argc != 2) {
                std::cerr << "Usage: " << argv[0] << " node_name" << std::endl;
                return 1;
        }

        char *nodeName = argv[1];

        auto connection = sdbus::createSystemBusConnection();
        auto controllerProxy = sdbus::createProxy(
                        *connection,
                        sdbus::ServiceName("org.eclipse.bluechi"),
                        sdbus::ObjectPath("/org/eclipse/bluechi"));

        sdbus::ObjectPath nodePath;
        controllerProxy->callMethod("GetNode")
                        .onInterface("org.eclipse.bluechi.Controller")
                        .withArguments(nodeName)
                        .storeResultsTo(nodePath);

        auto nodeProxy = sdbus::createProxy(
                        *connection, sdbus::ServiceName("org.eclipse.bluechi"), sdbus::ObjectPath(nodePath));

        std::vector<
                        sdbus::Struct< std::string,
                                       std::string,
                                       std::string,
                                       std::string,
                                       std::string,
                                       std::string,
                                       sdbus::ObjectPath,
                                       u_int32_t,
                                       std::string,
                                       sdbus::ObjectPath > >
                        units;
        nodeProxy->callMethod("ListUnits").onInterface("org.eclipse.bluechi.Node").storeResultsTo(units);

        for (auto &unit : units) {
                std::cout << unit.get< 0 >() << " - " << unit.get< 1 >() << std::endl;
        }

        return 0;
}
