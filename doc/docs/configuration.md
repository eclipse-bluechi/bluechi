# Configuration

For a list of supported options incl. an explanation please refer to the the MAN pages for
[bluechi-controller(5)](./man/bluechi-controller-conf.md) and [bluechi-agent(5)](./man/bluechi_agent_conf.md).

## Loading order

On startup, bluechi loads configuration files from the following directories:

| Load order | bluechi | bluechi-agent |
|---|---|---|
| 1 | `/usr/share/bluechi/config/controller.conf` | `/usr/share/bluechi-agent/config/agent.conf` |
| 2 | `/etc/bluechi/controller.conf` | `/etc/bluechi/agent.conf` |
| 3 | `/etc/bluechi/controller.conf.d` | `/etc/bluechi/agent.conf.d` |

Based on the load order, settings from a previously read configuration file will be overridden by subsequent files.
For example, the default setting for `AllowedNodeNames` in `/usr/share/bluechi/config/controller.conf` is an empty
list and can be overridden by either editing `/etc/bluechi/controller.conf` or adding a file in
`/etc/bluechi/controller.conf.d`. Configuration
files in `/etc/bluechi/controller.conf.d` are sorted alphabetically and read in ascending order.

It is also possible to pass the cli option `-c <path_to_file>` to both, bluechi and bluechi-agent. If specified, this
configuration has the highest priority and all defined settings will override previously set options.

## Maximum line length

The maximum line length supported by BlueChi is 500 characters. If the characters of any key-value pair exceeds this, use
multiple, indented lines. For example, a large number of node names in the `AllowedNodeNames` field can be split like this:

```bash
AllowedNodeNames=node1,
  node2,
  node3
```
