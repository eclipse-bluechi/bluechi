<!-- markdownlint-disable-file MD013 MD010 -->
# Getting started with Hirte

## Installing hirte

On Fedora systems, hirte can be directly installed from the Fedora repository.
On CentOS-Stream systems, it can be installed using the COPR repository shipping
the latest hirte code. That repository can be enabled using: dnf copr enable
mperina/hirte-snapshot centos-stream-9.  This should no longer be necessary once
hirte is made available in EPEL.

On the laptop install both hirte and the hirte-agent via this command:

```bash
dnf install hirte hirte-agent hirte-ctl
```

On the Raspberry Pi 4, install only the agent via this command:

```bash
dnf install hirte-agent
```

## Configuring hirte

Once hirte and its agents are installed, they need to be configured so the
agents know where the primary node is located and the hirte can identify each
node.

For both the laptop and the Raspberry Pi 4, the configuration files can be found
under: `/etc/hirte/`.

On the laptop, where both hirte and hirte-agent run, configure hirte in
`/etc/hirte/hirte.conf` as:

```ini
[hirte]
ManagerPort=2020
AllowedNodeNames=laptop,rpi4
```

Note the default port used by hirte is 808 which is considered a privileged
port, thus requiring it to be open in the firewall. To simplify this demo the
port 2020 is used, this non-privileged port does not require opening up the
firewall in Fedora if using the default settings.

Then configure the agent in `/etc/hirte/agent.conf` using:

```ini
[hirte-agent]
NodeName=laptop
ManagerHost=127.0.0.1
ManagerPort=2020
```

Note that the IP in the ManagerHost line can be either `127.0.0.1` (ie:
`localhost`) or the public IP address of the node: `192.168.42.10`.

On the Raspberry Pi 4, where only the agent is running, configure
`/etc/hirte/agent.conf` as:

```ini
[hirte-agent]
NodeName=rpi4
ManagerHost=192.168.42.10
ManagerPort=2020
```

In this case, the IP in the ManagerHost line refers to the IP address of the
laptop, since that's where hirte runs.

```text
Starting hirte and the agent
Starting hirte and the agent is now as simple as starting normal systemd services.
```

Run on the primary system (here the  laptop):

```bash
systemctl start hirte hirte-agent
```

And on the worker nodes (here the Raspberry Pi 4):

```bash
systemctl start hirte-agent
```

On each system, monitor the services using either, accordingly:

```bash
journalctl -lfu hirte
```

Or

```bash
journalctl -lfu hirte-agent
```

## Testing hirte

Once the services are up and running, the logs of the laptop show that the
Raspberry Pi successfully connects to the laptop:

```text
Mar 13 10:20:36 flame.pingoured.fr systemd[1]: Started hirte.service - Hirte service orchestrator manager daemon.
Mar 13 10:20:36 flame.pingoured.fr hirte[124510]: 10:20:36 INFO     ../src/manager/node.c:602 node_method_register      msg="Registered managed node from fd 8 as 'laptop'"
Mar 13 10:20:38 flame.pingoured.fr hirte[124510]: 10:20:38 INFO     ../src/manager/node.c:602 node_method_register      msg="Registered managed node from fd 9 as 'rpi4'"
```

From there, hirtectl can be used on the laptop to control services running on
either the laptop itself, or on the Raspberry Pi.

But before starting services, check which services are running:

```bash
# hirtectl list-units
NODE            	|ID                                                     	|   ACTIVE|  	SUB
====================================================================================================
laptop          	|time-sync.target                                       	| inactive| 	dead
laptop          	|nfs-idmapd.service                                     	| inactive| 	dead
laptop          	|sys-devices-platform-serial8250-tty-ttyS5.device       	|   active|  plugged
laptop          	|dev-disk-by\x2did-wwn\x2d0x5001b448b9db9490\x2dpart3.device|   active|  plugged
laptop          	|podman.socket                                          	|   active|listening
....
```

This returns the list of all the units and their state on all nodes. Restrict
the list to a certain node by running:

```bash
# hirtectl list-units rpi4
ID                                                                          	|   ACTIVE|  	SUB
====================================================================================================
systemd-update-done.service                                                 	|   active|   exited
boot.mount                                                                  	|   active|  mounted
dbus-broker.service                                                         	|   active|  running
system-getty.slice                                                          	|   active|   active
sshd-keygen@ecdsa.service                                                   	| inactive| 	dead
....
```

Check the status of httpd on the Raspberry Pi.

If nothing shows, it means httpd.service is not running, so start it:

```bash
# hirtectl start rpi4 httpd.service
```

On the Raspberry Pi, the logs of hirte-agent shows the service being started:

```bash
Mar 13 10:21:05 Host-002 hirte-agent[1556]: 10:21:05 INFO    	../src/agent/agent.c:836 agent_run_unit_lifecycle_method    	msg="Request to StartUnit unit: httpd.service - Action: replace"
```

And check outcome:

```bash
# hirtectl list-units rpi4 |grep httpd
httpd-init.service                                                          	| inactive| 	dead
httpd.service                                                               	|   active|  running
```

Use hirtectl to start, stop, restart or reload units, as shown above, it can
also be used to list units on a (or all) node(s). All of those can also be
performed independently, using hirte's D-Bus API. The repository on GitHub
contains a few examples of that D-Bus API.
