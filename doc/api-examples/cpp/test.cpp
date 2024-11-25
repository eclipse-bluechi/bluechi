#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <string>

int main() {
        const std::string nodeName = "laptop";
        const std::string unitName = "simple.service";

        try {
                // Create a connection to the system bus
                auto connection = sdbus::createSystemBusConnection();

                // Create a proxy for the controller interface
                auto controllerProxy = sdbus::createProxy(
                                *connection,
                                sdbus::ServiceName("org.eclipse.bluechi"),
                                sdbus::ObjectPath("/org/eclipse/bluechi"));

                // Call GetNode method on the controller proxy
                sdbus::ObjectPath nodePath;
                controllerProxy->callMethod("GetNode")
                                .onInterface("org.eclipse.bluechi.Controller")
                                .withArguments(nodeName)
                                .storeResultsTo(nodePath);

                // Create a proxy for the node interface
                auto nodeProxy = sdbus::createProxy(
                                *connection,
                                sdbus::ServiceName("org.eclipse.bluechi"),
                                sdbus::ObjectPath(nodePath));

                // Call StartUnit method on the node proxy
                sdbus::ObjectPath myJobPath;
                nodeProxy->callMethod("StartUnit")
                                .onInterface("org.eclipse.bluechi.Node")
                                .withArguments(unitName, "replace")
                                .storeResultsTo(myJobPath);

                // Print the result
                std::cout << "Started unit '" << unitName << "' on node '" << nodeName << "': " << myJobPath
                          << std::endl;
        } catch (const sdbus::Error &e) {
                std::cerr << "D-Bus error: " << e.what() << std::endl;
                return 1;
        }

        return 0;
}
