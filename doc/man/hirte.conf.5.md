% hirte.conf 5

## NAME

hirte.conf - Configuration file to bootstrap hirte

## DESCRIPTION

The basic file definition used to bootstrap hirte.

## Format

The hirte configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html). Contrary to this, there is no need for the `\` symbol at the end of a line to continue a value on the next line. A value continued on multiple lines will just be concatenated by hirte. The maximum line length supported is 500 characters. If the value exceeds this limitation, use multiple, indented lines.

### **hirte** section

All fields to bootstrap the hirte manager are contained in the **hirte** section. The following keys are understood by
`hirte`.

#### **ManagerPort** (uint16_t)

The port the manager listens on to establish connections with the `hirte-agent`. By default port `842` is used.

#### **AllowedNodeNames** (string)

A comma separated list of unique hirte-agent names. It's mandatory to set the option, only nodes with names mentioned
in the list can connect to `hirte` manager. These names are defined in the agent's configuration file under `NodeName`
option (see `hirte-agent.conf(5)`).

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

A basic example of a configuration file for `hirte`:

```
[hirte]
ManagerPort=842
AllowedNodeNames=agent-007,agent-123
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[hirte]
ManagerPort=842
AllowedNodeNames=agent-007,
   agent-123,
   agent-456,
   agent-789
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

## FILES

Distributions provide the __/usr/share/hirte/config/hirte.conf__ file which defines hirte configuration defaults. Administrators can copy this file to __/etc/hirte/hirte.conf__ and specify their own configuration.

Administrators can also use a "drop-in" directory __/etc/hirte/hirte.conf.d__ to drop their configuration changes.


## SEE ALSO

**[hirte(1)](https://github.com/containers/hirte/blob/main/doc/man/hirte.1.md)**
