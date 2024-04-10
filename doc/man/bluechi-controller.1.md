% bluechi 1

## NAME

bluechi-controller - Controller of services across agents

## SYNOPSIS

**bluechi-controller** [*options*]

## DESCRIPTION

Eclipse BlueChi™ is a systemd service controller intended for multi-nodes environments with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional safety (for example in cars).

The bluechi controller offers its public API on the local DBus and uses the DBus APIs provided by all connected `bluechi-agents` to manage systemd services on those remote systems.

**bluechi-controller [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--port**, **-p**

The port on which bluechi-controller is listening for and setting up connections with `bluechi-agents`. This option will overwrite the port defined in the configuration file.

#### **--config**, **-c**

Path to the configuration file, see `bluechi-controller.conf(5)`.

#### **--version**,  **-v**

Print current bluechi version

## Environment Variables

`bluechi-controller` understands the following environment variables that can be used to override the settings from the configuration file (see `bluechi-controller.conf(5)`).

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

If this flag is set to `true`, no logs are written by `bluechi`.

## Exit Codes

TBD

## CONFIGURATION FILES

**[bluechi-controller.conf(5)](https://github.com/eclipse-bluechi/bluechi/blob/main/doc/man/bluechi-controller.conf.5.md)**
