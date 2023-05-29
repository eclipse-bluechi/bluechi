% hirtectl 1

## NAME

hirtectl - Simple command line tool to interact with the public D-Bus API of hirte

## SYNOPSIS

**hirtectl** [*options*] *command*

## DESCRIPTION

A simple command line tool that uses the public D-Bus API provided by `hirte` to manage services on all connected `hirte-agents` and retrieve information from them.

**hirtectl [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

## Commands

### **hirtectl** [*start|stop|restart|reload*] [*agent*] [*unit*]

Performs one of the listed lifecycle operations on the given systemd unit for the `hirte-agent`.

### **hirtectl** [*enable|disable*] [*agent*] [*unit1*,*...*]

Enable/Disable the list of systemd unit files for the `hirte-agent`.

### **hirtectl** *list-units* [*agent*]

Fetches information about all systemd units on the hirte-agents. If [hirte-agent] is not specified, all agents are queried.

### **hirtectl** *monitor* [*agent*] [*unit1*,*unit2*,*...*]

Creates a monitor on the given agent to observe changes in the specified units. Wildcards **\*** to match all agents and/or units are also supported.

**Example:**

hirtectl monitor \\\* dbus.service,apache2.service

### **hirtectl** [*daemon-reload*] [*agent*]

Performs `daemon-reload` for the `hirte-agent`.
