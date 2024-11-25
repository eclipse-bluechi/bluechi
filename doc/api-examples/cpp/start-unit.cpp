/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

int main(int argc, char *argv[]) {
        if (argc != 3) {
                std::cerr << "Usage: " << argv[0] << " node_name unit_name" << std::endl;
                return 1;
        }

        char *nodeName = argv[1];
        char *unitName = argv[2];

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

        sdbus::ObjectPath myJobPath;
        nodeProxy->callMethod("StartUnit")
                        .onInterface("org.eclipse.bluechi.Node")
                        .withArguments(unitName, "replace")
                        .storeResultsTo(myJobPath);

        std::cout << "Started unit '" << unitName << "' on node '" << nodeName << "': " << myJobPath
                  << std::endl;

        return 0;
}
