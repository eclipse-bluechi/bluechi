% bluechi.conf 5

## NAME

bluechi-controller.conf - Configuration file to bootstrap bluechi-controller

## DESCRIPTION

The basic file definition used to bootstrap bluechi-controller.

## Format

The bluechi-controller configuration file is using the
[systemd configuration file format](https://www.freedesktop.org/software/systemd/man/systemd.syntax.html). Contrary to this, there is no need for the `\` symbol at the end of a line to continue a value on the next line. A value continued on multiple lines will just be concatenated by bluechi. The maximum line length supported is 500 characters. If the value exceeds this limitation, use multiple, indented lines.

### **bluechi-controller** section

All fields to bootstrap the bluechi controller are contained in the **bluechi-controller** section. The following keys are understood by `bluechi-controller`.

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

#### **IPReceiveErrors** (string)

If this flag is set to `true`, it enables extended, reliable error message passing for
the peer connection with all agents. This results in BlueChi receiving errors such as
host unreachable ICMP packets instantly and possibly dropping the connection. This is
useful to detect disconnects faster, but should be used with care as this might cause
unnecessary disconnects in less robut networks. Default: true.


## Example

A basic example of a configuration file for `bluechi`:

```
[bluechi-controller]
ManagerPort=842
AllowedNodeNames=agent-007,agent-123
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[bluechi-controller]
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

Distributions provide the __/usr/share/bluechi/config/controller.conf__ file which defines bluechi configuration defaults. Administrators can copy this file to __/etc/bluechi/controller.conf__ and specify their own configuration.

Administrators can also use a "drop-in" directory __/etc/bluechi/controller.conf.d__ to drop their configuration changes.


## SEE ALSO

**[bluechi-controller(1)](https://github.com/eclipse-bluechi/bluechi/blob/main/doc/man/bluechi-controller.1.md)**
