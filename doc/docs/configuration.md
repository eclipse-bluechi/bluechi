# Configuration

## Loading order

On startup, hirte loads configuration files from the following directories:

| Load order | hirte | hirte-agent |
|---|---|---|
| 1 | `/usr/share/hirte/config/hirte.conf` | `/usr/share/hirte-agent/config/agent.conf` |
| 2 | `/etc/hirte/hirte.conf` | `/etc/hirte/agent.conf` |
| 3 | `/etc/hirte/hirte.conf.d` | `/etc/hirte/agent.conf.d` |

Based on the load order, settings from a previously read configuration file will be overridden by subsequent files.
For example, the default setting for `AllowedNodeNames` in `/usr/share/hirte/config/hirte.conf` is an empty list and
can be overridden by either editing `/etc/hirte/hirte.conf` or adding a file in `/etc/hirte/hirte.conf.d`. Configuration
files in `/etc/hirte/hirte.conf.d` are sorted alphabetically and read in ascending order.

It is also possible to pass the cli option `-c <path_to_file>` to both, hirte and hirte-agent. If specified, this
configuration has the highest priority and all defined settings will override previously set options.

For a list of supported options incl. an explanation please refer to the
the MAN pages for [hirte(5)](./man/hirte_conf.md) and [hirte-agent(5)](./man/hirte_agent_conf.md).

## Maximum line length

The maximum line length supported by hirte is 500 characters. If the characters of any key-value pair exceeds this, use
multiple, indented lines. For example, a large number of node names in the `AllowedNodeNames` field can be split like this:

```bash
AllowedNodeNames=node1,
  node2,
  node3
```
