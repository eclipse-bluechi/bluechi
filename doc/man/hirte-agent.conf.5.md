% hirte-agent.conf 5

## NAME

agent.conf - Configuration file to bootstrap hirte-agent

## DESCRIPTION

The basic file definition used to bootstrap hirte-agent.

## Format

The hirte-agent configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html). Contrary to this, there is no need for the `\` symbol at the end of a line to continue a value on the next line. A value continued on multiple lines will just be concatenated by hirte. The maximum line length supported is 500 characters. If the value exceeds this limitation, use multiple, indented lines.

### **hirte-agent** section

All fields to bootstrap the hirte-agent are contained in the **hirte-agent** section. The following keys are understood
by `hirte-agent`.

#### **NodeName** (string)

The unique name of this agent. The option defaults to the system's hostname.

#### **ManagerAddress** (string)

SD Bus address used by `hirte-agent` to connect to `hirte`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ManagerHost` or `ManagerPort` defined in the configuration file as well as the respective CLI
options. The option doesn't have a default value.

#### **ManagerHost** (string)

The host used by `hirte-agent` to connect to `hirte`. Must be a valid IPv4 or IPv6. ManagerHost defaults to localhost 127.0.0.1. It's mandatory to set this field if the hirte agent is on a remote system.

#### **ManagerPort** (uint16_t)

The port on which `hirte` is listening for connection request and the `hirte-agent` is connecting to. By default port
`842` is used.

#### **HeartbeatInterval** (long)

The interval between two heartbeat signals sent to hirte in milliseconds. If an agent is not connected, it will retry to connect on each heartbeat. Setting this options to values smaller or equal to 0 disables it. This option will overwrite the heartbeat interval defined in the configuration file. 

#### **LogLevel** (string)

The level used for logging. Supported values are:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`

By default `INFO` level is used for logging.

#### **LogTarget** (string)

The target where logs are written to. Supported values are:

- `stderr`
- `stderr-full`
- `journald`

By default `journald` is used as the target.

#### **LogIsQuiet** (string)

If this flag is set to `true`, no logs are written by hirte. By default the flag is set to `false`.

## Example

Using `ManagerHost` and `ManagerPort` options:

```
[hirte-agent]
NodeName=agent-007
ManagerHost=127.0.0.1
ManagerPort=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using `ManagerAddress` option:

```
[hirte-agent]
NodeName=agent-007
ManagerAddress=tcp:host=127.0.0.1,port=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[hirte-agent]
NodeName=agent-007
ManagerAddress=tcp:
  host=127.0.0.1,
  port=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

## FILES

Distributions provide the __/usr/share/hirte/config/agent.conf__ file which defines hirte-agent configuration defaults. Administrators can copy this file to __/etc/hirte/agent.conf__ and specify their own configuration.

Administrators can also use a "drop-in" directory __/etc/hirte/agent.conf.d__ to drop their configuration changes.

## SEE ALSO

**[hirte-agent(1)](https://github.com/containers/hirte/blob/main/doc/man/hirte-agent.1.md)**
