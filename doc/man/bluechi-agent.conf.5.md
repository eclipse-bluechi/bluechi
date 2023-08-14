% bluechi-agent.conf 5

## NAME

agent.conf - Configuration file to bootstrap bluechi-agent

## DESCRIPTION

The basic file definition used to bootstrap bluechi-agent.

## Format

The bluechi-agent configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html). Contrary to this, there is no need for the `\` symbol at the end of a line to continue a value on the next line. A value continued on multiple lines will just be concatenated by bluechi. The maximum line length supported is 500 characters. If the value exceeds this limitation, use multiple, indented lines.

### **bluechi-agent** section

All fields to bootstrap the bluechi-agent are contained in the **bluechi-agent** section. The following keys are understood
by `bluechi-agent`.

#### **NodeName** (string)

The unique name of this agent. The option defaults to the system's hostname.

#### **ManagerAddress** (string)

SD Bus address used by `bluechi-agent` to connect to `bluechi`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ManagerHost` or `ManagerPort` defined in the configuration file as well as the respective CLI
options. The option doesn't have a default value.

#### **ManagerHost** (string)

The host used by `bluechi-agent` to connect to `bluechi`. Must be a valid IPv4 or IPv6. ManagerHost defaults to localhost 127.0.0.1. It's mandatory to set this field if the bluechi agent is on a remote system.

#### **ManagerPort** (uint16_t)

The port on which `bluechi` is listening for connection request and the `bluechi-agent` is connecting to. By default port
`842` is used.

#### **HeartbeatInterval** (long)

The interval between two heartbeat signals sent to bluechi in milliseconds. If an agent is not connected, it will retry to connect on each heartbeat. Setting this options to values smaller or equal to 0 disables it. This option will overwrite the heartbeat interval defined in the configuration file. 

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

If this flag is set to `true`, no logs are written by bluechi. By default the flag is set to `false`.

## Example

Using `ManagerHost` and `ManagerPort` options:

```
[bluechi-agent]
NodeName=agent-007
ManagerHost=127.0.0.1
ManagerPort=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using `ManagerAddress` option:

```
[bluechi-agent]
NodeName=agent-007
ManagerAddress=tcp:host=127.0.0.1,port=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[bluechi-agent]
NodeName=agent-007
ManagerAddress=tcp:
  host=127.0.0.1,
  port=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

## FILES

Distributions provide the __/usr/share/bluechi/config/agent.conf__ file which defines bluechi-agent configuration defaults. Administrators can copy this file to __/etc/bluechi/agent.conf__ and specify their own configuration.

Administrators can also use a "drop-in" directory __/etc/bluechi/agent.conf.d__ to drop their configuration changes.

## SEE ALSO

**[bluechi-agent(1)](https://github.com/containers/bluechi/blob/main/doc/man/bluechi-agent.1.md)**
