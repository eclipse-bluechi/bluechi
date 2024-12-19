<!-- markdownlint-disable-file MD010 MD013 MD014 MD024 MD046 -->

# Setup: Multi node

BlueChi is intended for multi-node environments with a predefined number of nodes. This section describes how to set it up based on an example with two machines - a laptop and a raspberry pi. The diagram below depicts the desired state of the system:

![BlueChi multi node setup diagram](../assets/img/bluechi_setup_multi_node.png)

## Installation and configuration

The main node will be the **laptop**. So first of all, let's install the controller, the agent and the CLI tool as well as the SELinux policy on it:

```bash
dnf install bluechi bluechi-agent bluechi-ctl bluechi-selinux
```

The **raspberry pi** will be (one of) the managed node in this example. Therefore, only install the agent and the SELinux policy on it:

```bash
dnf install bluechi-agent bluechi-selinux
```

Once the installations succeeded, BlueChi needs to be configured on both machines.

### Configuring the main node

Let's start with the main node (the **laptop** in this case). The configuration of the controller running on the **laptop** is similar to the [single node setup](#installation-and-configuration) with two exceptions:

- **pi** needs to be added as another node
- **pi** is a remote agent and will connect to BlueChi via TCP/IP. By default, the privileged port 842 is used. To avoid any firewall issues for this demo, let's overwrite this setting and use the port **2020**.

```bash
echo -e "[bluechi-controller]\nControllerPort=2020\nAllowedNodeNames=$(hostname),pi\n" > /etc/bluechi/controller.conf.d/1.conf
```

Let's also create the configuration for the local agent on the **laptop** to use the Unix Domain Socket:

```bash
echo -e "[bluechi-agent]\nControllerAddress=unix:path=/run/bluechi/bluechi.sock\n" > /etc/bluechi/agent.conf.d/1.conf
```

### Configuring the managed node

In the next step the managed node (the **raspberry pi** in this case) needs to be configured. The managed node needs to know where to connect to, so the settings for the [ControllerHost](../man/bluechi-agent-conf.md#controllerhost-string) as well as the [ControllerPort](../man/bluechi-agent-conf.md#controllerport-uint16_t) have to be set to the IP and Port of the main node. In addition, the [NodeName](../man/bluechi-agent-conf.md#nodename-string) has to be set to **pi** (as set in the **AllowedNodeNames** in the [previous section](#configuring-the-main-node)).

Assuming the main nodes IP within the network is `192.168.42.10`, the configuration file for the agent on the **raspberry pi** can be created like this:

```bash
echo -e "[bluechi-agent]\nControllerHost=192.168.42.10\nControllerPort=2020\n" > /etc/bluechi/agent.conf.d/1.conf
```

The content of the new file should be:

```bash
$ cat /etc/bluechi/agent.conf.d/1.conf

[bluechi-controller]
ControllerHost=192.168.42.10
ControllerPort=2020
```

## Running BlueChi

After [installation and configuration](#installation-and-configuration) has been completed, let's start the systemd services.

Starting the controller and agent on the main node (**laptop**):

```bash
systemctl start bluechi-controller bluechi-agent
```

Switching to the managed node (**raspberry pi**), start the agent:

```bash
systemctl start bluechi-agent
```

Once the services are up and running, the journald logs on the **laptop** should show that the both agents have successfully connected to the controller:

```bash
$ journalctl -u bluechi-controller
Dec 18 08:03:03 localhost systemd[1]: Started BlueChi Controller systemd service
Dec 18 08:03:03 localhost bluechi-controller[680]: Starting bluechi-controller 0.10.0-0
Dec 17 16:13:43 localhost bluechi-controller[2539]: Waiting for connection requests on port 842...
Dec 17 16:13:43 localhost bluechi-controller[2539]: Waiting for connection requests on socket /run/bluechi/bluechi.sock...
Dec 18 08:03:03 localhost bluechi-controller[680]: Heartbeat disabled since configured interval '0' is <=0
Dec 18 08:03:03 localhost bluechi-controller[680]: Registered managed node from fd 9 as 'local'
Dec 18 08:03:03 localhost bluechi-controller[680]: Registered managed node from fd 10 as 'pi'
...
```

Let's use `bluechictl` on the main node to list all units:

```bash
# list all units on the laptop
$ bluechictl list-units laptop

NODE            	|ID                                                     	|   ACTIVE|  	SUB
====================================================================================================
laptop          	|time-sync.target                                       	| inactive| 	dead
laptop          	|nfs-idmapd.service                                     	| inactive| 	dead
laptop          	|sys-devices-platform-serial8250-tty-ttyS5.device       	|   active|  plugged
laptop          	|dev-disk-by\x2did-wwn\x2d0x5001b448b9db9490\x2dpart3.device|   active|  plugged
laptop          	|podman.socket                                          	|   active|listening
...

# list all units on the raspberry pi
$ bluechictl list-units pi

NODE            	|ID                                                     	|   ACTIVE|  	SUB
====================================================================================================
pi                  |acpid.path                                                 |   active|  running
pi                  |system-systemd\x2dcryptsetup.slice                         |   active|   active
pi                  |nfs-kernel-server.service                                  | inactive|     dead
pi                  |dev-ttyS24.device                                          |   active|  plugged
pi                  |dbus.service                                               |   active|  running
...
```

The setup works!

For more examples on how to use BlueChi, please see refer to the [examples section](./examples_bluechictl.md).

!!! Note

    For a full list of available commands of `bluechictl`, please refer to [bluechictl(1)](../man/bluechictl.md).
