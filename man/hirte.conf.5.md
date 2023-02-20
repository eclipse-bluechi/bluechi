% hirte.conf 5

## NAME

hirte.conf - Configuration file to bootstrap hirte

## DESCRIPTION

The basic file definition used to bootstrap hirte.

## Format

The hirte configuration file is using the .ini file format.

### Manager section

All fields to bootstrap the hirte manager are contained in the **Manager** group. The following keys are understood by `hirte`.

#### **Port** (uint16_t)

The port the manager listens on to establish connections with the `hirte-agents`.

#### **Nodes** (string)

A comma separated list of unique hirte-agent names. These are used as a whitelist to validate any register request from a `hirte-agent`.

### Logging section

Fields in the **Logging** group define the logging behavior of `hirte`. These settings are overridden by the environment variables (see `hirte(1)`) for logging.

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
[Manager]
Port=2020
Nodes=agent-007,agent-123

[Logging]
Level=DEBUG
Target=journald
Quiet=false
```

## SEE ALSO

**[hirte(1)](https://github.com/containers/hirte/blob/main/man/hirte.1.md)**
