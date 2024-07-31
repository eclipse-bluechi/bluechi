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

#### **ControllerAddress** (string)

SD Bus address used by `bluechi-agent` to connect to `bluechi`. See `man sd_bus_set_address` for its format.
Overrides any setting of `ControllerHost` or `ControllerPort` defined in the configuration file as well as the respective CLI
options. The option doesn't have a default value.

#### **ControllerHost** (string)

The host used by `bluechi-agent` to connect to `bluechi`. Must be a valid IPv4 or IPv6. ControllerHost defaults to localhost 127.0.0.1. It's mandatory to set this field if the bluechi agent is on a remote system.

#### **ControllerPort** (uint16_t)

The port on which `bluechi` is listening for connection request and the `bluechi-agent` is connecting to. By default port
`842` is used.

#### **HeartbeatInterval** (long)

The interval between two heartbeat signals sent to bluechi in milliseconds. If an agent is not connected, it will retry to connect on each heartbeat. Setting this options to values smaller or equal to 0 disables it. This option will overwrite the heartbeat interval defined in the configuration file.

#### **ControllerHeartbeatThreshold** (long)

The threshold in milliseconds to determine whether a bluechi agent is disconnected. If the controller's last heartbeat signal was received before this threshold, bluechi agent assumes that the controller is down or the connection was cut off and performs a disconnect.

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
the peer connection with the controller. This results in BlueChi receiving errors such as
host unreachable ICMP packets instantly and possibly dropping the connection. This is
useful to detect disconnects faster, but should be used with care as this might cause
unnecessary disconnects in less robut networks.
Default: true.

#### **TCPKeepAliveTime** (long)

The number of seconds the TCP connection of the agent with the controller needs to be idle before
keepalive packets are sent. When `TCPKeepAliveTime` is set to 0, the system default will be used.
Default: 1s.

#### **TCPKeepAliveInterval** (long)

The number of seconds between each keepalive packet. When `TCPKeepAliveInterval` is set to 0,
the system default will be used.
Default: 1s.

#### **TCPKeepAliveCount** (long)

The number of keepalive packets without ACK from the controller till the connection is
dropped. When `TCPKeepAliveCount` is set to 0, the system default will be used.
Default: 6.


## Example

Using `ControllerHost` and `ControllerPort` options:

```
[bluechi-agent]
NodeName=agent-007
ControllerHost=127.0.0.1
ControllerPort=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using `ControllerAddress` option:

```
[bluechi-agent]
NodeName=agent-007
ControllerAddress=tcp:host=127.0.0.1,port=842
LogLevel=DEBUG
LogTarget=journald
LogIsQuiet=false
```

Using a value that is continued on multiple lines:

```
[bluechi-agent]
NodeName=agent-007
ControllerAddress=tcp:
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

**[bluechi-agent(1)](https://github.com/eclipse-bluechi/bluechi/blob/main/doc/man/bluechi-agent.1.md)**
