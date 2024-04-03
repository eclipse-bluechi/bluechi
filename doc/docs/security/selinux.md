<!-- markdownlint-disable-file MD013-->
# BlueChi's SELinux policy

BlueChi provides a custom SELinux policy, limiting access of the `bluechi-controller` and `bluechi-agent`. It can be installed via

```bash
dnf install bluechi-selinux
```

By default, the package allows `bluechi-controller` to bind and `bluechi-agent` to connect to any port so that changes of the [ControllerPort](../man/bluechi-agent-conf.md#controllerport-uint16_t) are not being blocked.

## Enforce port restrictions

In order to allow BlueChi to only use one port, the usage of any needs to be disabled with [setsebool](https://linux.die.net/man/8/setsebool):

```bash
# Turn off the any policy for bluechi-controller
setsebool -P bluechi_controller_port_bind_any 0

# Turn off the any policy for bluechi-agent
setsebool -P bluechi_agent_port_connect_any 0
```

Subsequently, the desired port needs to be allowed by using [semanage](https://linux.die.net/man/8/semanage):

```bash
# Set the allowed port for bluechi-controller
semanage port -a -t bluechi_port_t -p tcp <port>

# Set the allowed port for bluechi-agent
semanage port -a -t bluechi_agent_port_t -p tcp <port>
```

## Change from enforcing to permissive

By default, BlueChi will enforce its SELinux policy. By using [semanage](https://linux.die.net/man/8/semanage) the permissive property can be added so that violations are blocked and create only an AVC entry:

```bash
# add the permissive property to bluechi-controller
semanage permissive -a bluechi_t

# add the permissive property to bluechi-agent
semanage permissive -a bluechi_agent_t
```
