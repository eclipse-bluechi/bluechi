<!-- markdownlint-disable-file MD010 MD013 MD014 MD024 MD046 -->

# Examples on how to use BlueChi

For the examples in this section, assume a [multi-node setup](./multi_node.md) that uses [bluechictl](../man/bluechictl.md) to interact with BlueChi is running. To leverage the full potential of BlueChi, e.g. by writing custom applications, please refer to the section describing how to [use BlueChi's D-Bus API](../api/examples.md).

## Getting information of units

The `bluechictl list-units` command can be used to get a list of all units and their states on either all nodes or the specified node. It also provides a filter option for the unit name:

```bash
$ bluechictl list-units --filter=bluechi*

NODE                |ID                                                         |   ACTIVE|      SUB
====================================================================================================
laptop              |bluechi-controller.service                                 |   active|  running
laptop              |bluechi-agent.service                                      |   active|  running
pi                  |bluechi-agent.service                                      |   active|  running
```

## Monitoring of units

The `bluechictl monitor <node-name> <unit-name>` command enables to view changes in real-time. For example, to monitor all state changes of `cow.service` on `pi` the following command can be issued:

```bash
$ bluechictl monitor pi cow.service
```

`bluechictl monitor` also supports the wildcard character `*` for both, `<node-name>` and `<unit-name>`.

```bash
$ bluechictl monitor \* \*

Monitor path: /org/eclipse/bluechi/monitor/1
Subscribing to node '*' and unit '*'
[laptop] *
	Unit created (reason: virtual)
[pi] *
	Unit created (reason: virtual)
...
```

!!! Note

    When a unit subscription with a wildcard `*` exists, BlueChi emits `virtual` events for the unit with name `*` on

    - creation of the wildcard subscription
    - node in the the wildcard subscription dis-/connects

    This enables an observer to do the necessary re-queries since state changes could have happened while the node was disconnected.

## Monitoring of nodes

In addition to monitoring units, BlueChi's APIs can be used to query and monitor the node connection states:

```bash
$ bluechictl status

NODE                          | STATE     | LAST SEEN
=========================================================================
laptop                        | online    | now
pi                            | online    | now
```

Assuming node `pi` would go offline, the output would immediately change to something like this:

```bash
NODE                          | STATE     | LAST SEEN
=========================================================================
laptop                        | online    | now
pi                            | online    | 2023-10-06 08:38:20,000+0200
```

It is also possible to show the status of a specific node

```bash
$ bluechictl status laptop

NODE                          | STATE     | LAST SEEN
=========================================================================
laptop                        | online    | now
```

In addition, a flag `-w/--watch` can be used with `bluechictl status` to continuously display the nodes status, refreshing on node status change.

## Operations on units

The `bluechictl start` command can be used to start systemd units on managed nodes:

```bash
$ bluechictl start pi httpd

Done
```

!!! Note

    Starting a systemd unit via BlueChi's D-Bus API queues a job to start that unit in systemd. `bluechictl start` also waits for this job to be completed. This also applies to other operations like **stop**.

Let's stop the `httpd` service:

```bash
$ bluechictl stop pi httpd

Done
```

Assuming the new systemd unit **cow.service** has been created on the managed node **pi**, systemd requires a daemon reload to use it. This can also be triggered via `bluechictl`:

```bash
$ bluechictl daemon-reload pi
$ bluechictl start pi cow.service

Done
```

If it is required to enable/disable the **cow.service**, this can easily be done via:

```bash
$ bluechictl enable pi cow.service
$ bluechictl disable pi cow.service
```

If it is required to temporarily prevent the **cow.service** to get any CPU time, this can be done via a `freeze` command:

```bash
$ bluechictl freeze pi cow.service

# revert the previous freeze
$ bluechictl thaw pi cow.service
```

## Print unit status

The `bluechictl status <node-name> <unit-name>` will print the specific unit info and status

```bash
$ bluechictl status laptop httpd.service

UNIT            | LOADED    | ACTIVE    | SUBSTATE  | FREEZERSTATE  | ENABLED   |
---------------------------------------------------------------------------------
httpd.service   | loaded    | active    | running   | running       | enabled   |

```
