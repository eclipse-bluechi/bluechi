% hirte-agent.conf 5

## NAME

hirte-agent.conf - Configuration file to bootstrap hirte-agent

## DESCRIPTION

The basic file definition used to bootstrap hirte-agent.

## Format

The hirte-agent configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html).

### **hirte-agent** section

All fields to bootstrap the hirte-agent are contained in the **hirte-agent** section. The following keys are understood by `hirte-agent`.

#### **NodeName** (string)

The unique name of this agent.

#### **ManagerAddress** (string)

SD Bus address used by `hirte-agent` to connect to `hirte`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ManagerHost` or `ManagerPort` defined in the configuration file as well as the respective CLI options.

#### **ManagerHost** (string)

The host used by `hirte-agent` to connect to `hirte`. Must be a valid IPv4 or IPv6.

#### **ManagerPort** (uint16_t)

The port on which `hirte` is listening for connection request and the `hirte-agent` is connecting to.

#### **LogLevel** (string)

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

#### **LogTarget** (string)

The target where logs are written to. Supported values are:

- `stderr`
- `stderr-full`
- `journald`

#### **LogIsQuiet** (string)

If this flag is set to `true`, no logs are written by hirte.

## Example

```
[hirte-agent]
NodeName=agent-007
ManagerHost=127.0.0.1
ManagerPort=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

## SEE ALSO

**[hirte-agent(1)](https://github.com/containers/hirte/blob/main/doc/man/hirte-agent.1.md)**
