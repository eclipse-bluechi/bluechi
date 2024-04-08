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

#### **ControllerPort** (uint16_t)

The port the bluechi-controller listens on to establish connections with the `bluechi-agent`. By default port `842` is used.

#### **AllowedNodeNames** (string)

A comma separated list of unique bluechi-agent names. It's mandatory to set the option, only nodes with names mentioned
in the list can connect to `bluechi-controller'. These names are defined in the agent's configuration file under `NodeName`
option (see `bluechi-agent.conf(5)`).

#### **HeartbeatInterval** (long)

The interval to periodically check node's connectivity based on heartbeat signals sent to bluechi, in milliseconds.

#### **NodeHeartbeatThreshold** (long)

The threshold in milliseconds to determine whether a node is disconnected. If the node's last heartbeat signal was received before this threshold, bluechi treats that the node has been disconnected.

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
unnecessary disconnects in less robut networks.
Default: true.

#### **TCPKeepAliveTime** (long)

The number of seconds the individual TCP connection with an agent needs to be idle
before keepalive packets are sent. When `TCPKeepAliveTime` is set to 0, the system
default will be used.
Default: 1s.

#### **TCPKeepAliveInterval** (long)

The number of seconds between each keepalive packet. When `TCPKeepAliveInterval` is set to 0,
the system default will be used.
Default: 1s.

#### **TCPKeepAliveCount** (long)

The number of keepalive packets without ACK from an agent till the individual connection is dropped.
When `TCPKeepAliveCount` is set to 0, the system default will be used.
Default: 6.

## Example

A basic example of a configuration file for `bluechi`:

```
[bluechi-controller]
ControllerPort=842
AllowedNodeNames=agent-007,agent-123
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[bluechi-controller]
ControllerPort=842
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
