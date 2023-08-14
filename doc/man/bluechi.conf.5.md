% bluechi.conf 5

## NAME

bluechi.conf - Configuration file to bootstrap bluechi

## DESCRIPTION

The basic file definition used to bootstrap bluechi.

## Format

The bluechi configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html). Contrary to this, there is no need for the `\` symbol at the end of a line to continue a value on the next line. A value continued on multiple lines will just be concatenated by bluechi. The maximum line length supported is 500 characters. If the value exceeds this limitation, use multiple, indented lines.

### **bluechi** section

All fields to bootstrap the bluechi manager are contained in the **bluechi** section. The following keys are understood by
`bluechi`.

#### **ManagerPort** (uint16_t)

The port the manager listens on to establish connections with the `bluechi-agent`. By default port `842` is used.

#### **AllowedNodeNames** (string)

A comma separated list of unique bluechi-agent names. It's mandatory to set the option, only nodes with names mentioned
in the list can connect to `bluechi` manager. These names are defined in the agent's configuration file under `NodeName`
option (see `bluechi-agent.conf(5)`).

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

A basic example of a configuration file for `bluechi`:

```
[bluechi]
ManagerPort=842
AllowedNodeNames=agent-007,agent-123
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[bluechi]
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

Distributions provide the __/usr/share/bluechi/config/bluechi.conf__ file which defines bluechi configuration defaults. Administrators can copy this file to __/etc/bluechi/bluechi.conf__ and specify their own configuration.

Administrators can also use a "drop-in" directory __/etc/bluechi/bluechi.conf.d__ to drop their configuration changes.


## SEE ALSO

**[bluechi(1)](https://github.com/containers/bluechi/blob/main/doc/man/bluechi.1.md)**
