# Eclipse BlueChi&trade;

## What is Eclipse BlueChi?

Eclipse BlueChi (formerly known as hirte) is a deterministic multi-node service controller.

BlueChi can manage the states of different services across multiple nodes with a
focus on highly regulated industries, such as those requiring functional safety.
BlueChi integrates with systemd via its D-Bus API and relays D-Bus messages over
TCP for multi-nodes support.

On the main node a service called `bluechi-controller` is running. On startup, the controller
loads the configuration files which describe all the involved systems
(called nodes) that are expected to be managed. Each node has a
unique name that is used to reference it in the controller.

On each managed node that is under control of BlueChi a service called `bluechi-agent`
is running. When the service starts, it connects (via D-Bus over TCP) to the controller
and registers as available (optionally authenticating). It then receives requests
from the controller and reports local state changes to it.

## Background

BlueChi is built around three components:

- `bluechi-controller` service, running on the primary node, controls all connected nodes
- `bluechi-agent` services, with one running on each managed node, is the agent
  talking locally to systemd to act on services
- `bluechictl` command line program, is meant to be used by administrators to test,
  debug, or manually manage services across nodes.

BlueChi is meant to be used in conjunction with a state manager (a program or
person) knowing the desired state of the system(s). This design choice has a few
consequences:

BlueChi itself does not know the desired final state of the
system(s), it only knows how to transition between states, i.e.: how to start,
stop, restart a service on one or more nodes.

BlueChi monitors and reports
changes in services running, so that another application like a state manager is notified when a
service stops running or when the connection to a node is lost, but BlueChi itself
does not act on these notifications.

BlueChi does not handle the “initial
setup” of the system, it is assumed that the system boots in a desired state and
BlueChi handles the transitions from this state.
