/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

int main(int argc, char *argv[]) {
        if (argc != 4) {
                std::cerr << "Usage: " << argv[0] << " node_name unit_name cpu_value" << std::endl;
                return 1;
        }

        char *nodeName = argv[1];
        char *unitName = argv[2];
        char *cpuWeight = argv[3];

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

        std::vector< sdbus::Struct< std::string, sdbus::Variant > > values;
        u_int64_t val = std::stoi(cpuWeight);
        auto newValue = sdbus::Struct< std::string, sdbus::Variant >("CPUWeight", sdbus::Variant{ val });
        values.push_back(newValue);

        nodeProxy->callMethod("SetUnitProperties")
                        .onInterface("org.eclipse.bluechi.Node")
                        .withArguments(unitName, false, values);

        return 0;
}
