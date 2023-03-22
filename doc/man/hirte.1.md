% hirte 1

## NAME

hirte - Manager of services across agents

## SYNOPSIS

**hirte** [*options*]

## DESCRIPTION

Hirte is a systemd service controller intended for multi-nodes environments with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional safety (for example in cars).

The hirte manager offers its public API on the local DBus and uses the DBus APIs provided by all connected `hirte-agents` to manage systemd services on those remote systems.

**hirte [OPTIONS]**

## OPTIONS

#### **--help**, **-h**

Print usage statement and exit.

#### **--port**, **-p**

The port on which hirte is listening for and setting up connections with `hirte-agents`. This option will overwrite the port defined in the configuration file.

#### **--config**, **-c**

Path to the configuration file, see `hirte.conf(5)`.

## Environment Variables

`hirte` understands the following environment variables that can be used to override the settings from the configuration file (see `hirte.conf(5)`).

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

If this flag is set to `true`, no logs are written by `hirte`.

## Exit Codes

TBD

## CONFIGURATION FILES

TBD

## SEE ALSO

**[hirte.conf(5)](https://github.com/containers/hirte/blob/main/doc/man/hirte.conf.5.md)**
