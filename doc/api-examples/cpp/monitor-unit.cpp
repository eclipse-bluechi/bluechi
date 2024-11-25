/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>

static int on_unit_new(sdbus::Signal signal) {
        try {
                std::string node, unit, reason;
                signal >> node;
                signal >> unit;
                signal >> reason;

                std::cout << "New Unit " << unit << " on node " << node << ", reason: " << reason << std::endl;
        } catch (const sdbus::Error &e) {
                std::cerr << "D-Bus error: " << e.what() << std::endl;
        }

        return 0;
}

static int on_unit_removed(sdbus::Signal signal) {
        try {
                std::string node, unit, reason;
                signal >> node;
                signal >> unit;
                signal >> reason;

                std::cout << "Removed Unit " << unit << " on node " << node << ", reason: " << reason
                          << std::endl;
        } catch (const sdbus::Error &e) {
                std::cerr << "D-Bus error: " << e.what() << std::endl;
        }

        return 0;
}

static int on_unit_props_changed(sdbus::Signal signal) {
        try {
                std::string node, unit, iface;
                signal >> node;
                signal >> unit;
                signal >> iface;

                std::cout << "Unit " << unit << " on node " << node << " changed for iface " << iface
                          << std::endl;
        } catch (const sdbus::Error &e) {
                std::cerr << "D-Bus error: " << e.what() << std::endl;
        }

        return 0;
}

static int on_unit_state_changed(sdbus::Signal signal) {
        try {
                std::string node, unit, activeState, subState, reason;
                signal >> node;
                signal >> unit;
                signal >> activeState;
                signal >> subState;
                signal >> reason;

                std::cout << "Unit " << unit << " on node " << node << " changed to state (" << activeState
                          << ", " << subState << "), reason: " << reason << std::endl;
        } catch (const sdbus::Error &e) {
                std::cerr << "D-Bus error: " << e.what() << std::endl;
        }

        return 0;
}

int main(int argc, char *argv[]) {
        if (argc != 2) {
                std::cerr << "Usage: " << argv[0] << " node_name" << std::endl;
                return 1;
        }
        auto nodeName = argv[1];

        auto connection = sdbus::createSystemBusConnection();
        auto controllerProxy = sdbus::createProxy(
                        *connection,
                        sdbus::ServiceName("org.eclipse.bluechi"),
                        sdbus::ObjectPath("/org/eclipse/bluechi"));

        sdbus::ObjectPath monitorPath;
        controllerProxy->callMethod("CreateMonitor")
                        .onInterface("org.eclipse.bluechi.Controller")
                        .storeResultsTo(monitorPath);

        auto monitorProxy = sdbus::createProxy(
                        *connection, sdbus::ServiceName("org.eclipse.bluechi"), monitorPath);

        monitorProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitNew"),
                        on_unit_new);
        monitorProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitRemoved"),
                        on_unit_removed);
        monitorProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitPropertiesChanged"),
                        on_unit_props_changed);
        monitorProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitStateChanged"),
                        on_unit_state_changed);

        monitorProxy->callMethod("Subscribe")
                        .onInterface("org.eclipse.bluechi.Monitor")
                        .withArguments(nodeName, "*");

        connection->enterEventLoop();
        return 0;
}


/*
agentProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitRemoved"),
                        on_properties_changed);
        agentProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitPropertiesChanged"),
                        on_properties_changed);
        agentProxy->registerSignalHandler(
                        sdbus::InterfaceName("org.eclipse.bluechi.Monitor"),
                        sdbus::SignalName("UnitStateChanged"),
                        on_properties_changed);
*/
