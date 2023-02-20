% hirte-agent.conf 5

## NAME

hirte-agent.conf - Configuration file to bootstrap hirte-agent

## DESCRIPTION

The basic file definition used to bootstrap hirte-agent.

## Format

The hirte-agent configuration file is using the .ini file format.

### Node section

All fields to bootstrap the hirte-agent are contained in the **Node** group. The following keys are understood by `hirte-agent`.

#### **Name** (string)

The unique name of this agent.

#### **Host** (string)

The host used by `hirte-agent` to connect to `hirte`. Must be a valid IPv4 or IPv6.

#### **Port** (uint16_t)

The port on which `hirte` is listening for connection request and the `hirte-agent` is connecting to.

### Logging section

Fields in the **Logging** group define the logging behavior of `hirte-agent`. These settings are overridden by the environment variables (see `hirte-agent(1)`) for logging.

#### **Level** (string)

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

#### **Target** (string)

The target where logs are written to. Supported values are:

- `stderr`
- `journald`

#### **Quiet** (string)

If this flag is set to `true`, no logs are written by hirte.

## Example

```
[Node]
Name=agent-007
Host=127.0.0.1
Port=2020

[Logging]
Level=DEBUG
Target=journald
Quiet=false
```

## SEE ALSO

**[hirte-agent(1)](https://github.com/containers/hirte/blob/main/man/hirte-agent.1.md)**
