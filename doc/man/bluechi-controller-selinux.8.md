% bluechi-controller.selinux 8

## NAME

bluechi-controller.selinux - Security Enhanced Linux Policy for the `bluechi-controller` processes

## DESCRIPTION

Security-Enhanced Linux secures the bluechi-controller processes via flexible mandatory access control.

The bluechi processes execute with the `bluechi_t` SELinux type. You can check if you have these processes running by executing the **ps** command with the **-Z** qualifier.

For example:

    ps -eZ \| grep bluechi_t

## ENTRYPOINTS

The bluechi_t SELinux type can be entered via the **bluechi_exec_t** file type.

The default entrypoint paths for the bluechi_t domain are the following:

/usr/bin/bluechi-controller\
/usr/libexec/bluechi-controller

## PROCESS TYPES

SELinux defines process types (domains) for each process running on the system

You can see the context of a process using the **-Z** option to **ps**

Policy governs the access confined processes have to files. SELinux bluechi policy is very flexible allowing users to setup their bluechi processes in as secure a method as possible.

The following process types are defined for bluechi:

- `bluechi_t` 
- `bluechi_agent_t`

Note: **semanage permissive -a bluechi_t** can be used to make the process type bluechi_t permissive. SELinux does not deny access to permissive process types, but the AVC (SELinux denials) messages are still generated.

## BOOLEANS

SELinux policy is customizable based on least access required. bluechi policy is extremely flexible and has several booleans that allow you to manipulate the policy and run bluechi with the tightest access possible.

If you want to dontaudit all daemons scheduling requests (setsched, sys_nice), you must turn on the daemons_dontaudit_scheduling boolean. Enabled by default.

    setsebool -P daemons_dontaudit_scheduling 1

If you want to allow all domains to execute in fips_mode, you must turn on the fips_mode boolean. Enabled by default.

    setsebool -P fips_mode 1

## PORT TYPES

SELinux defines port types to represent TCP and UDP ports.

You can see the types associated with a port by using the following command:

    semanage port -l

Policy governs the access confined processes have to these ports. SELinux bluechi policy is very flexible allowing users to setup their bluechi processes in as secure a method as possible.

The following port types are defined for bluechi:

`bluechi_port_t`

Default Defined Ports:\
udp 842

## MANAGED FILES

The SELinux process type bluechi_t can manage files labeled with the following file types. The paths listed are the default paths for these file types. Note the processes UID still need to have DAC permissions.

\
**cluster_conf_t**

/etc/cluster(/.\*)?

\
**cluster_var_lib_t**

/var/lib/pcsd(/.\*)?\
/var/lib/cluster(/.\*)?\
/var/lib/openais(/.\*)?\
/var/lib/pengine(/.\*)?\
/var/lib/corosync(/.\*)?\
/usr/lib/heartbeat(/.\*)?\
/var/lib/heartbeat(/.\*)?\
/var/lib/pacemaker(/.\*)?

\
**cluster_var_run_t**

/var/run/crm(/.\*)?\
/var/run/cman\_.\*\
/var/run/rsctmp(/.\*)?\
/var/run/aisexec.\*\
/var/run/heartbeat(/.\*)?\
/var/run/pcsd-ruby.socket\
/var/run/corosync-qnetd(/.\*)?\
/var/run/corosync-qdevice(/.\*)?\
/var/run/corosync.pid\
/var/run/cpglockd.pid\
/var/run/rgmanager.pid\
/var/run/cluster/rgmanager.sk

\
**initrc_tmp_t**

\
**mnt_t**

/mnt(/\[\^/\]\*)?\
/mnt(/\[\^/\]\*)?\
/rhev(/\[\^/\]\*)?\
/rhev/\[\^/\]\*/.\*\
/media(/\[\^/\]\*)?\
/media(/\[\^/\]\*)?\
/media/.hal-.\*\
/var/run/media(/\[\^/\]\*)?\
/afs\
/net\
/misc\
/rhev

\
**root_t**

/sysroot/ostree/deploy/.\*-atomic/deploy(/.\*)?\
/\
/initrd

\
**tmp_t**

/sandbox(/.\*)?\
/tmp\
/usr/tmp\
/var/tmp\
/var/tmp\
/tmp-inst\
/var/tmp-inst\
/var/tmp/tmp-inst\
/var/tmp/vi.recover

# FILE CONTEXTS

SELinux requires files to have an extended attribute to define the file type.

You can see the context of a file using the **-Z** option to **ls**

Policy governs the access confined processes have to these files. SELinux bluechi policy is very flexible allowing users to setup their bluechi processes in as secure a method as possible.

#### STANDARD FILE CONTEXT

SELinux defines the file context types for the bluechi, if you wanted to store files with these types in a different paths, you need to execute the semanage command to specify alternate labeling and then use restorecon to put the labels on disk.

```
semanage fcontext -a -t bluechi_exec_t '/srv/bluechi/content(/.*)?'
restorecon -R -v /srv/mybluechi_content
```
Note: SELinux often uses regular expressions to specify labels that match multiple files.

*The following file types are defined for bluechi:*

`bluechi_agent_exec_t`

\- Set files with the bluechi_agent_exec_t type, if you want to transition an executable to the bluechi_agent_t domain.

`bluechi_exec_t`

\- Set files with the bluechi_exec_t type, if you want to transition an executable to the bluechi_t domain.

Note: File context can be temporarily modified with the chcon command. If you want to permanently change the file context you need to use the **semanage fcontext** command. This will modify the SELinux labeling database. You will need to use **restorecon** to apply the labels.

## COMMANDS

`semanage fcontext` can also be used to manipulate default file context mappings.

`semanage permissive` can also be used to manipulate whether or not a process type is permissive.

`semanage module` can also be used to enable/disable/install/remove policy modules.

`semanage port` can also be used to manipulate the port definitions

`semanage boolean` can also be used to manipulate the booleans

`system-config-selinux` is a GUI tool available to customize SELinux policy settings.

## AUTHOR

This manual page was auto-generated using **sepolicy manpage**.

## SEE ALSO

selinux(8), bluechi-controller(8), semanage(8), restorecon(8), chcon(1), sepolicy(8), setsebool(8), bluechi_agent_selinux(8), bluechi_controller_selinux(8)
