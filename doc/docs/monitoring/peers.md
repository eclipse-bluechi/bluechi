<!-- markdownlint-disable-file MD013 MD014 MD046 -->

# Peer listeners

By default, only the monitor creator receives the events of the subscribed units from BlueChi. Peer listeners are other applications that did not create the monitor itself, but still want to get those messages. They can be added to any previously set up monitor via the BlueChi API.

The systemd policy of BlueChi defines that only the `root` user (by default) can fully utilize the API. Therefore, an application run from a different user is not able to call any methods on BlueChi. It is still able to receive events from a monitor if it is added as peer by a `root` user application.

## Using peer listeners

This section assumes at least a [single-node setup](../getting_started/single_node.md) and will use the [generated python bindings](../api/client_generation.md#typed-python-client).

First, let's start `bluechi-controller` and `bluechi-agent`:

```bash
$ systemctl start bluechi-controller
$ systemctl start bluechi-agent
```

The `create-and-listen.py` (see further down) can be used to easily create a monitor as well as one subscription. In the following snippet the `cow.service` on the node `laptop` is being monitored:

```bash
$ python create-and-listen.py laptop cow.service
Monitor path: /org/eclipse/bluechi/monitor/1
```

It will print the path of the monitor. This path is required to attach event listener to it. Let's open a new terminal and start the listening script:

```bash
$ python only-listen.py /org/eclipse/bluechi/monitor/1
Using unique name: :1.9327
```

The first line in the output is the unique bus name of the application. This is the name used to add it as a peer to the previously created monitor.

Before adding it as peer, however, let's check the status and/or do start and stop operations on the unit that is monitored:

```bash
$ bluechictl status laptop cow.service
$ bluechictl start laptop cow.service
$ bluechictl stop laptop cow.service
$ ...
```

As a result, the `create-and-listen.py` application should have received a few events:

```bash
Unit New: laptop -- cow.service
Unit Removed: laptop -- cow.service
Unit New: laptop -- cow.service
Unit state changed: laptop -- cow.service
Unit props changed: laptop -- cow.service
Unit props changed: laptop -- cow.service
...
```

Contrary, the `only-listen.py` should not have received anything yet. Let's change that by adding its bus name to the monitor as peer:

```bash
$ python add-peer.py /org/eclipse/bluechi/monitor/1 :1.9327
Added :1.9327 to monitor /org/eclipse/bluechi/monitor/1 as peer with ID 1
```

When checking the status of the unit or starting and stopping it now, `create-and-listen.py` as well as `only-listen.py` receive the events emitted by BlueChi.

=== "create-and-listen.py"

    ```python
    import sys
    from bluechi.api import Controller, Monitor
    from dasbus.loop import EventLoop

    if len(sys.argv) != 3:
        print("Usage: python create-and-listen.py <node-name> <unit-name>")
        sys.exit(1)

    node_name = sys.argv[1]
    unit_name = sys.argv[2]

    loop = EventLoop()

    mgr = Controller()
    monitor_path = mgr.create_monitor()
    print(f"Monitor path: {monitor_path}")

    monitor = Monitor(monitor_path)

    def on_new(node, unit, reason):
        print(f"Unit New: {node} -- {unit}")

    def on_removed(node, unit, reason):
        print(f"Unit Removed: {node} -- {unit}")

    def on_state(node, unit, active, sub, reason):
        print(f"Unit state changed: {node} -- {unit}")

    def on_props(node, unit, interface, props):
        print(f"Unit props changed: {node} -- {unit}")

    monitor.on_unit_new(on_new)
    monitor.on_unit_removed(on_removed)
    monitor.on_unit_state_changed(on_state)
    monitor.on_unit_properties_changed(on_props)

    monitor.subscribe(node_name, unit_name)

    loop.run()
    ```

=== "only-listen.py"

    ```python
    import sys
    from bluechi.api import Monitor
    from dasbus.loop import EventLoop

    if len(sys.argv) != 2:
        print("Usage: python listen.py <monitor-path>")
        sys.exit(1)

    monitor_path = sys.argv[1]
    monitor = Monitor(monitor_path)

    loop = EventLoop()

    def on_new(node, unit, reason):
        print(f"Unit New: {node} -- {unit}")

    def on_removed(node, unit, reason):
        print(f"Unit Removed: {node} -- {unit}")

    def on_state(node, unit, active, sub, reason):
        print(f"Unit state changed: {node} -- {unit}")

    def on_props(node, unit, interface, props):
        print(f"Unit props changed: {node} -- {unit}")

    monitor.on_unit_new(on_new)
    monitor.on_unit_removed(on_removed)
    monitor.on_unit_state_changed(on_state)
    monitor.on_unit_properties_changed(on_props)


    def on_removed(reason):
        print(f"Removed, reason: {reason}")
        loop.quit()

    monitor.get_proxy().PeerRemoved.connect(on_removed)

    print(f"Using unique name: {monitor.bus.connection.get_unique_name()}")

    loop.run()
    ```

=== "add-peer.py"

    ```python
    import sys
    from bluechi.api import Monitor

    if len(sys.argv) != 3:
        print("Usage: python add-peer.py <monitor-path> <bus-name>")
        sys.exit(1)

    monitor_path = sys.argv[1]
    bus_name = sys.argv[2]

    monitor = Monitor(monitor_path)
    peer_id = monitor.get_proxy().AddPeer(bus_name)
    print(f"Added {bus_name} to monitor {monitor_path} as peer with ID {peer_id}")
    ```

!!! Note

    The Peer API will be introduced in BlueChi v0.7.0 so that there will be easy to use helper functions adding a peer (as in `add-peer.py`) and listening for the peer removed signal (as in `listen-only.py`).
