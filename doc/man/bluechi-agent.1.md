% bluechi-agent 1

## NAME

bluechi-agent - Agent managing services on the local machine

## SYNOPSIS

**bluechi-agent** [*options*]

## DESCRIPTION

Eclipse BlueChi™ is a systemd service controller intended for multi-node environment with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional safety (for example in cars).

A `bluechi-agent` establishes a peer-to-peer connection to `bluechi` and exposes its API to manage systemd units on it.

**bluechi-agent [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--host**, **-H**

The host used by `bluechi-agent` to connect to `bluechi`. Must be a valid IPv4 or IPv6. This option will overwrite the host defined in the configuration file.

#### **--port**, **-p**

The port on which `bluechi` is listening for connection request and the `bluechi-agent` is connecting to. This option will overwrite the port defined in the configuration file. (default: 842)

#### **--address**, **-a**

DBus address used by `bluechi-agent` to connect to `bluechi`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ControllerHost` or `ControllerPort` defined in the configuration file as well as the respective CLI options.

#### **--name**, **-n**

The unique name of this `bluechi-agent` used for registering at `bluechi`. This option will overwrite the port defined in the configuration file.

#### **--interval**, **-i**

The interval between two heartbeat signals sent to bluechi in milliseconds. If an agent is not connected, it will retry to connect on each heartbeat. Setting this options to values smaller or equal to 0 disables it. This option will overwrite the heartbeat interval defined in the configuration file.

#### **--config**, **-c**

Path to the configuration file, see `bluechi-agent.conf(5)`. (default: /etc/bluechi/agent.conf)

#### **--version**,  **-v**

Print current bluechi-agent version

#### **--user**

Connect to the user systemd instance instead of the system one.

## Environment Variables

`bluechi-agent` understands the following environment variables that can be used to override the settings from the configuration file (see `bluechi-agent.conf(5)`).

#### **BC_LOG_LEVEL**

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

#### **BC_LOG_TARGET**

The target where logs are written to. Supported values are:

- `stderr`
- `stderr-full`
- `journald`

#### **BC_LOG_IS_QUIET**

If this flag is set to `true`, no logs are written by bluechi.

## Exit Codes

TBD

## CONFIGURATION FILES

**[bluechi-agent.conf(5)](https://github.com/eclipse-bluechi/bluechi/blob/main/doc/man/bluechi-agent.conf.5.md)**
