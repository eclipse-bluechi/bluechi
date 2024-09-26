% bluechictl 1

## NAME

bluechictl - Simple command line tool to interact with the public D-Bus API of bluechi

## SYNOPSIS

**bluechictl** [*options*] *command*

## DESCRIPTION

A simple command line tool that uses the public D-Bus API provided by Eclipse BlueChi™ to manage services on all connected `bluechi-agents` and retrieve information from them.

**bluechictl [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--version**,  **-v**

Print current bluechictl version

## Commands

### **bluechictl** [*start|stop|freeze|thaw|restart|reload*] [*agent*] [*unit*]

Performs one of the listed lifecycle operations on the given systemd unit for the `bluechi-agent`.

### **bluechictl** [*kill*] [*agent*] [*unit*]

Kills the processes of (i.e. sends a signal to) the specified unit on the chosen node.

**Options:**

**--kill-whom**
    Enum defining which processes of the unit are killed.
    Needs to be one of [all, main, control]. Default: all

**--signal**
    The signal sent to kill the processes of the unit.
    Default: 15 (SIGTERM)


### **bluechictl** [*enable*] [*agent*] [*unit1*,*...*]

Enable the list of systemd unit files for the `bluechi-agent`.


**Options:**

**--force**, **-f**
    Override existing symlinks

**--runtime**
    Enable unit files temporarily until next reboot

**--no-reload**
    Don't reload daemon after enabling unit files

### **bluechictl** [*disable*] [*agent*] [*unit1*,*...*]

Disable the list of systemd unit files for the `bluechi-agent`.


**Options:**

**--no-reload**
    Don't reload daemon after disabling unit files

### **bluechictl** *list-unit-files* [*agent*]

Fetches information about all systemd unit files on the bluechi-agents. If [bluechi-agent] is not specified, all agents are queried.

**Options:**

**--filter**
    Use glob filter for the unit file path

### **bluechictl** *is-enabled* [*agent*] [*unit*]

Fetches the current enablement status of the specific unit file on the specific `bluechi-agent`.

### **bluechictl** *list-units* [*agent*]

Fetches information about all systemd units on the bluechi-agents. If [bluechi-agent] is not specified, all agents are queried.

**Options:**

**--filter**
    Use glob filter for the unit names

### **bluechictl** *monitor* [*agent*] [*unit1*,*unit2*,*...*]

Creates a monitor on the given agent to observe changes in the specified units. Wildcards **\*** to match all agents and/or units are also supported.


**Example:**

bluechictl monitor

bluechictl monitor node1

bluechictl monitor \\\* dbus.service,apache2.service

### **bluechictl** *daemon-reload* [*agent*]

Performs `daemon-reload` for the `bluechi-agent`.

### **bluechictl** *status* [*agent*]

Fetches the status of all the agents or a specific agent: state (online/offline) and when was it last seen


**Options:**

**--watch**, **-w**
    Continuously display agent(s) status, updating when state change update received


**Example:**

bluechictl status

bluechictl status rpi

bluechictl status -w

### **bluechictl** *status* [*agent*] [*unit1*,*...*]

Fetches the status of the systemd units for the `bluechi-agent`.

### **bluechictl** *reset-failed* [*agent*] [*unit1*,*...*]

Performs a `reset-failed` on the chosen `bluechi-agent` for the selected units.

### **bluechictl** *get-default* [*agent*] 

Fetches the default target on the chosen `bluechi-agent`.

### **bluechictl** *set-default* [*agent*] [TARGET]

Changes the default target to `TARGET` file on the chosen `bluechi-agent`.
