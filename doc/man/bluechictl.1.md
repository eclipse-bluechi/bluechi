% bluechictl 1

## NAME

bluechictl - Simple command line tool to interact with the public D-Bus API of bluechi

## SYNOPSIS

**bluechictl** [*options*] *command*

## DESCRIPTION

A simple command line tool that uses the public D-Bus API provided by `bluechi` to manage services on all connected `bluechi-agents` and retrieve information from them.

**bluechictl [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--version**,  **-v**

Print current bluechictl version

## Commands

### **bluechictl** [*start|stop|freeze|thaw|restart|reload*] [*agent*] [*unit*]

Performs one of the listed lifecycle operations on the given systemd unit for the `bluechi-agent`.

### **bluechictl** [*enable|disable*] [*agent*] [*unit1*,*...*]

Enable/Disable the list of systemd unit files for the `bluechi-agent`.

### **bluechictl** *list-units* [*agent*]

Fetches information about all systemd units on the bluechi-agents. If [bluechi-agent] is not specified, all agents are queried.

### **bluechictl** *monitor* [*agent*] [*unit1*,*unit2*,*...*]

Creates a monitor on the given agent to observe changes in the specified units. Wildcards **\*** to match all agents and/or units are also supported.

### **bluechictl** *monitor* *node-connection*

Creates a monitor to observe connection state changes for all nodes.

### **bluechictl** *system-resources* *agent*

Fetches information about system resources on the specific bluechi-agents.


**Example:**

bluechictl monitor

bluechictl monitor node1

bluechictl monitor \\\* dbus.service,apache2.service

### **bluechictl** *daemon-reload* [*agent*]

Performs `daemon-reload` for the `bluechi-agent`.

### **bluechictl** [*status*] [*agent*] [*unit1*,*...*]

Fetches the status of the systemd units for the `bluechi-agent`.
