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

## Allowing access to restricted units

The SELinux policy of BlueChi allows it to manage all systemd units.

However, when installing some applications and their respective systemd units, e.g. via `dnf`, there might also be additional SELinux Policies installed which prevent BlueChi from managing these. For example, when installing `httpd` on Fedora, it installs also the systemd unit `httpd.service` and the [policy module for apache](https://github.com/fedora-selinux/selinux-policy/blob/rawhide/policy/modules/contrib/apache.te). When trying to start the service via `bluechictl`, the policy will prevent certain operations:

```bash
# The apache policy prevents BlueChi from managing the httpd service
$ bluechictl stop <node-name> httpd.service
Failed to issue method call: SELinux policy denies access: Permission denied

# However, using systemctl works as this is enabled by default in the apache policy
$ systemctl stop httpd.service
```

In order to allow BlueChi to manage the `httpd.service`, the necessary allow rule(s) need to be added.

### Allowing access using audit2allow

The [audit2allow](https://man7.org/linux/man-pages/man1/audit2allow.1.html) tool can be used to generate these rules based on AVCs.

First, run all operations with BlueChi that should be allowed. These will fail and create the AVCs used by `audit2allow`. Then use `audit2allow` to generate the allow rules and create the policy package (.pp) which can be installed via `semodule`. The following snippet shows an example for the httpd.service:

```bash
# generate AVC for the status operation
$ bluechictl status <node-name> httpd.service
Failed to issue method call: SELinux policy denies access: Permission denied

# generate AVC for the stop operation
$ bluechictl stop <node-name> httpd.service
Failed to issue method call: SELinux policy denies access: Permission denied

# view the type enforcement rule that allows the denied operations
$ audit2allow -a
#============= bluechi_agent_t ==============

allow bluechi_agent_t httpd_unit_file_t:service { status stop };

# create the policy package (.pp) and type enforcement file (.te)
$ audit2allow -a -M httpd-allow
******************** IMPORTANT ***********************
To make this policy package active, execute:

semodule -i httpd-allow.pp

$ ls
httpd-allow.pp httpd-allow.te

# install policy package and run the previously prevented operation
$ semodule -i httpd-allow.pp
$ bluechictl status <node-name> httpd.service
UNIT		    | LOADED	| ACTIVE	| SUBSTATE	| FREEZERSTATE	| ENABLED	|
---------------------------------------------------------------------------------
httpd.service	| loaded	| inactive	| dead		| running	    | disabled	|
```

### Allowing access by custom policy

If you want to grant BlueChi full access, then writing your custom policy is faster. For `httpd` it might look like this:

```bash
$ cat httpd-allow.te

module my 1.0;

require {
	type httpd_unit_file_t;
	type bluechi_agent_t;
    class service { reload start status stop kill load };
}

#============= bluechi_agent_t ==============
allow bluechi_agent_t httpd_unit_file_t:service { start stop status reload kill load } ;
```

Then install the `selinux-policy-devel` package, compile the `.te` file and install the generated package policy:

```bash
# install required packages
$ dnf install selinux-policy-devel -y

# compile .te file
$ make -f /usr/share/selinux/devel/Makefile httpd-allow.pp
Compiling targeted httpd-allow module
Creating targeted httpd-allow.pp policy package
rm tmp/httpd-allow.mod.fc tmp/httpd-allow.mod

# install compiled policy package
semodule -i httpd-allow.pp
```
