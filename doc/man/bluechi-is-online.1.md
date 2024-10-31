% bluechi-is-online 1

## NAME

bluechi-is-online - Command line tool to check and monitor the connection state of BlueChi's core components.

## SYNOPSIS

**bluechi-is-online** [*system|node|agent*] [*options*]

## DESCRIPTION

Eclipse BlueChi™ is a systemd service controller intended for multi-node environment with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional safety (for example in cars).

`bluechi-is-online` is a command line tool to check and monitor the connection state of BlueChi's core components. It uses the public D-Bus API provided by `bluechi-controller` and/or `bluechi-agent`.

**bluechi-is-online [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--version**, **-v**

Print current bluechictl version

## Commands

### **bluechi-is-online** *system* [*options*]

Checks the connection state of `bluechi-controller` and all expected `bluechi-agent`s. Runs on primary node.
Uses the D-Bus property `Status` of the `org.eclipse.bluechi.Controller` interface.
If the `Status` is `up`, exit code is 0. Otherwise 1.

**Options:**

**--wait**
Initial time in milliseconds to wait until the expected system status is `up`.

**--monitor**
If set, `bluechi-is-online` continuously monitors the system state after the initial `up` check succeeded.

### **bluechi-is-online** *node* [*node_name*] [*options*]

Checks the connection state of a specific node. Runs on primary node.
Uses the D-Bus property `Status` of the `org.eclipse.bluechi.Node` interface.
If the `Status` is `online`, exit code is 0. Otherwise 1.

**Options:**

**--wait**
Initial time in milliseconds to wait until the expected node status is `online`.

**--monitor**
If set, `bluechi-is-online` continuously monitors the node state after the initial `online` check succeeded.

### **bluechi-is-online** *agent* [*options*]

Checks the connection state of the `bluechi-agent` with the controller. Runs on any managed node.
Uses the D-Bus property `Status` of the `org.eclipse.bluechi.Agent` interface.
If the `Status` is `online`, exit code is 0. Otherwise 1.

**Options:**

**--wait**
Initial time in milliseconds to wait until the expected agent status is `online`.

**--monitor**
If set, `bluechi-is-online` continuously monitors the agent state after the initial `online` check succeeded.

**--switch-timeout**
Time to wait until the agent is expected to connect again on a call to the `SwitchController` API method. In milliseconds.

## Exit Codes

If the respective BlueChi component is online at the time of exiting, 0 is returned. Otherwise, 1 is returned.
