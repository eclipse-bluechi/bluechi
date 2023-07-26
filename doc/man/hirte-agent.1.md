% hirte-agent 1

## NAME

hirte-agent - Agent managing services on the local machine

## SYNOPSIS

**hirte-agent** [*options*]

## DESCRIPTION

Hirte is a systemd service controller intended for multi-node environment with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional safety (for example in cars).

A `hirte-agent` establishes a peer-to-peer connection to `hirte` and exposes its API to manage systemd units on it.

**hirte-agent [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--host**, **-H**

The host used by `hirte-agent` to connect to `hirte`. Must be a valid IPv4 or IPv6. This option will overwrite the host defined in the configuration file.

#### **--port**, **-p**

The port on which `hirte` is listening for connection request and the `hirte-agent` is connecting to. This option will overwrite the port defined in the configuration file. (default: 842)

#### **--address**, **-a**

DBus address used by `hirte-agent` to connect to `hirte`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ManagerHost` or `ManagerPort` defined in the configuration file as well as the respective CLI options.

#### **--name**, **-n**

The unique name of this `hirte-agent` used for registering at `hirte`. This option will overwrite the port defined in the configuration file.

#### **--interval**, **-i**

The interval between two heartbeat signals sent to hirte in milliseconds. If an agent is not connected, it will retry to connect on each heartbeat. Setting this options to values smaller or equal to 0 disables it. This option will overwrite the heartbeat interval defined in the configuration file. 

#### **--config**, **-c**

Path to the configuration file, see `hirte-agent.conf(5)`. (default: /etc/hirte/agent.conf)

#### **--user**

Connect to the user systemd instance instead of the system one.

## Environment Variables

`hirte-agent` understands the following environment variables that can be used to override the settings from the configuration file (see `hirte-agent.conf(5)`).

#### **HIRTE_LOG_LEVEL**

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

#### **HIRTE_LOG_TARGET**

The target where logs are written to. Supported values are:

- `stderr`
- `stderr-full`
- `journald`

#### **HIRTE_LOG_IS_QUIET**

If this flag is set to `true`, no logs are written by hirte.

## Exit Codes

TBD

## CONFIGURATION FILES

**[hirte-agent.conf(5)](https://github.com/containers/hirte/blob/main/doc/man/hirte-agent.conf.5.md)**
