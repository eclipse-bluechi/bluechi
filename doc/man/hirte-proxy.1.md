% hirte-proxy 1

## NAME

hirte-proxy - Proxy requesting services on other agents

## SYNOPSIS

**hirte-proxy** [*options*] *command*

## DESCRIPTION

Hirte is a service orchestrator tool intended for multi-node devices
(e.g.: edge devices) clusters with a predefined number of nodes and
with a focus on highly regulated environment such as those requiring
functional safety (for example in cars).

A `hirte-proxy` uses the public API of `hirte-agent` provided on the
local D-Bus to request services required by a local unit to be started
or stopped on a remote node managed by another `hirte-agent`.

**hirte-proxy [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--version**,  **-v**

Print current hirte-proxy version


## Commands

### **hirte-proxy** create [*node_service*]

Starts the `hirte-proxy` to call the `CreateProxy` method on the local
`hirte-agent` to request a remote service to be started on the given
node. `hirte-proxy` is exiting when starting the service has either
succeeded or failed.

### **hirte-proxy** remove [*node_service*]

Starts the `hirte-proxy` to call the `RemoveProxy` method on the local
`hirte-agent` to request a remote service to be started on the given
node. `hirte-proxy` is exiting when starting the service has either
succeeded or failed.

## Exit Codes

On successfully starting the remote service, 0 is returned. Otherwise, 1 is returned.

## SEE ALSO

**[systemd.unit(5)](https://www.freedesktop.org/software/systemd/man/systemd.unit.html)**
