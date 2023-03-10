% hirte.conf 5

## NAME

hirte.conf - Configuration file to bootstrap hirte

## DESCRIPTION

The basic file definition used to bootstrap hirte.

## Format

The hirte configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html).

### **hirte** section

All fields to bootstrap the hirte manager are contained in the **hirte** section. The following keys are understood by `hirte`.

#### **ManagerPort** (uint16_t)

The port the manager listens on to establish connections with the `hirte-agent`.

#### **AllowedNodeNames** (string)

A comma separated list of unique hirte-agent names. Only nodes with names mentioned in the list can connect to `hirte` manager. These names are defined in the agent's configuration file under `NodeName` option (see `hirte-agent.conf(5)`).

#### **LogLevel** (string)

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

#### **LogTarget** (string)

The target where logs are written to. Supported values are:

- `stderr`
- `journald`

#### **LogIsQuiet** (string)

If this flag is set to `true`, no logs are written by hirte.

## Example

```
[hirte]
ManagerPort=2020
AllowedNodeNames=agent-007,agent-123
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

## SEE ALSO

**[hirte(1)](https://github.com/containers/hirte/blob/main/doc/man/hirte.1.md)**
