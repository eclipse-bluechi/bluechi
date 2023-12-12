<!-- markdownlint-disable-file MD013 MD033 -->
# Monitoring

In a distributed environment it is essential to be able to access the state of applications as well as getting notified if it changes. This enables listeners to take appropriate actions, e.g. restarting a crashed service. For this purpose an API for monitoring systemd services on managed nodes is provided by BlueChi.

!!! Note

    BlueChi itself does not act upon any monitored change. It just forwards such events to other, external applications that requested them.

## How BlueChi implements service monitoring

In general, the `bluechi-agent` listens for unit-specific signals emitted by `systemd` on a managed node. If such a signal is received and a monitor with a subscription matching the unit is present, the agent will forward that message to the `bluechi-controller`. In turn, the controller is assembling an appropriate message and sends it to the creator of the monitor as well as to all monitoring peers. The peer concept is explained in more detail in the section [Peers listeners](./peers.md).

The Getting Started guide contains a few examples on how `bluechictl` can be used to [monitor units and nodes](../getting_started/examples_bluechictl.md#monitoring-of-units-and-nodes). If you would like to implement your custom application, e.g. for automatic state transitions of a system, please have a look at the [API example for monitoring units](../api/examples.md#monitor-unit-changes).

## Real vs virtual events

As described previously, BlueChi will forward unit-specific signals from `systemd`. Signals resulting from those events are called real events with `reason=real`. In a distributed environment, agent nodes might lose connection to the controller node and reconnect later. While disconnected, changes to monitored units might happen. The state an external monitoring application would diverge from the actual state if only real events were used. To address this issue, BlueChi also emits virtual events with `reason=virtual`. The [table of events](#list-of-events) flags each signal if there are also virtual events emitted.

## List of events

The following table shows and briefly explains all events that BlueChi listens on and forwards to listeners:

<center>

| Event | Summary | Has virtual events? |
|---|---|:-:|
| `UnitNew`  | Sent each time a unit is loaded by systemd | yes |
| `UnitRemoved`  | Sent each time a unit is unloaded by systemd | yes |
| `UnitStateChanged`  | Sent each time the unit state changed, e.g. from running to failed | yes |
| `UnitPropertiesChanged` | Sent each time at least one property of the unit changed, e.g. when setting the `CPUWeight` property | no |

</center>

!!! Note

    Both, `UnitNew` and `UnitRemoved`, are emitted by systemd when the status of the unit is actively queried and has to be loaded. For example, if the unit is in an inactive state and the `systemctl status` command or similar is executed, both events will be emitted once. In order to detect if a unit has successfully started, its best to use the `UnitStateChanged` event.

All events a monitor can emit and methods that can be invoked are listed in the [introspection data of the monitor](https://github.com/eclipse-bluechi/bluechi/blob/main/data/org.eclipse.bluechi.Monitor.xml).
