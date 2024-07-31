<!-- markdownlint-disable-file MD013 MD007 MD024 -->
# D-Bus API Description

The main way to interact with BlueChi is using [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/). It exposes a name on the system bus called `org.eclipse.bluechi` that other programs can use to control the system. It is expected that high-level control planes use this API directly, but there is also debugging tool called [bluechictl](../man/bluechictl.md) which can be used for debugging and testing purposes.

The interfaces described in this sections are referencing the [introspection data](https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format) which is defined in XML files located in the [data directory](https://github.com/eclipse-bluechi/bluechi/tree/main/data) of the project.

## BlueChi public D-Bus API

The main entry point is at the `/org/eclipse/bluechi` object path and implements the `org.eclipse.bluechi.Controller`
interface.

Note that some properties also come with change events, so you can easily track when they change.

### interface org.eclipse.bluechi.Controller

#### Methods

  * `ListUnits(out a(sssssssouso) units)`

    Returns an array with all currently loaded systemd units on all online nodes. This is equivalent to calling
    `Node.ListUnits()` on all the online nodes and adding the name of the node as the first element of each returned
    element struct.

  * `CreateMonitor(out o monitor)`

    Creates a new monitor object, which can be used to monitor the state of selected units on the nodes. The monitor
    object returned will automatically be closed if the calling peer disconnects from the bus.

  * `ListNodes(out a(soss) nodes)`

    Returns information (name, object_path, status and peer IP) of all known nodes.

  * `GetNode(in s name, out o path)`

    Returns the object path of a node given its name.

  * `EnableMetrics()`

    Enables the collection of metrics on all connected agents.

  * `DisableMetrics()`

    Disables the collection of metrics on all agents.

  * `SetLogLevel(in s log_level)`

    Set the new log level for bluechi controller. This change is persistent as long as bluechi not restarted.

#### Signals

  * `JobNew(u id, o job, s node_name, s unit)`

    Emitted each time a new BlueChi job is queued.

  * `JobRemoved(u id, o job, s node_name, s unit, s result)`

    Emitted each time a new job is dequeued or the underlying systemd job finished. `result` is one of: `done`,
    `failed`, `cancelled`, `timeout`, `dependency`, `skipped`. This is either the result from systemd on the node, or
    `cancelled` if the job was cancelled in BlueChi before any systemd job was started for it.

#### Properties

  * `Nodes` - `as`

    A list with the names of all configured nodes managed by BlueChi. Each name listed here also has a corresponding
    object under `/org/eclipse/bluechi/node/$name` which implements the `org.eclipse.bluechi.Node` interface.

### interface org.eclipse.bluechi.Monitor

Object path: `/org/eclipse/bluechi/monitor/$id`

#### Methods

  * `Close()`

    Stop monitoring and delete the Monitor object, removing any outstanding subscriptions.

  * `Subscribe(in node s, in unit s, out id u)`

    Subscribe to changes in properties of a given unit on a given node. Passing a wildcard `*` to either `node` or
    `unit` will subscribe to all nodes or all units on the given node. This will emit the signal `UnitChanged` for all
    matching units in the system, and then again whenever one of the properties of the unit changes. Returns an
    identifier `id` used for a subsequent `Unsubscribe`.

  * `Unsubscribe(in id u)`

    Remove an earlier added subscription.

  * `SubscribeList(in targets a(ss))`

    Subscribe to changes in properties of the given targets, consisting of node-unit-pairs. A wildcard `*` can be used
    to subscribe to changes on all nodes or all units on the given node. This will emit the signal `UnitChanged`
    for all matching units in the system, and then again whenever one of the properties of the unit changes. Returns an
    identifier `id` used for a subsequent `Unsubscribe`.

#### Signals

  * `UnitPropertiesChanged(s node, s unit, s interface, a{sv} props)`

    Whenever the properties changes for any of the units that are currently subscribed to changes, this signal is
    emitted. Additionally it is emitted initially once for each matched unit. This allows you to easily monitor and get
    the current state in a race-free way without missing any changes.

  * `UnitNew(s node, s unit, s reason)`

     Emitted when a new unit is loaded by systemd, for example when a
     service is started (reason=`real`), or if bluechi learns of an
     already loaded unit (reason=`virtual`).

    The latter can happen for two reasons, either bluechi already knows
    that the unit is loaded. Or, at a later time a new agent connects
    to a previously offline node and the unit was already running
    on the node.

  * `UnitStateChanged(s node, s unit, s active_state, s substate, s reason)`

    Emitted when the active state (and substate) of a monitored unit
    changes. Additionally, when a new subscription is added to a unit that
    is already active, a virtual event is sent. This makes it very easy
    to track the current active state of a unit.

  * `UnitRemove(s node, s unit, s reason)`

    Emitted when a unit is unloaded by systemd (reason=`real`), or
    when the agent disconnects and we previously reported the unit
    as loaded (reason=`virtual`).

### interface org.eclipse.bluechi.Node

Each node object represents a configured node in the system, independent of whether that node is connected to the
controller or not, and the status can change over time.

Object path: `/org/eclipse/bluechi/node/$name`

#### Methods

  * `StartUnit(in s name, in s mode, out o job)`

    Queues a unit activate job for the named unit on this node. The queue is per-unit name, which means there is only
    ever one active job per unit. Mode can be one of `replace` or `fail`. If there is an outstanding queued (but not
    running) job, that is replaced if mode is `replace`, or the job fails if mode is `fail`.

    The job returned is an object path for an object implementing `org.eclipse.bluechi.Job`, and which be monitored for
    the progress of the job, or used to cancel the job. To track the result of the job, follow the `JobRemoved` signal
    on the Controller.

  * `StopUnit(in s name, in s mode, out o job)`

    `StopUnit()` is similar to `StartUnit()` but stops the specified unit rather than starting it.

  * `ReloadUnit(in  s name, in  s mode, out o job)`

  * `RestartUnit(in  s name, in  s mode, out o job)`

    `ReloadUnit()`/`RestartUnit()` is similar to `StartUnit()` but can be used to reload/restart a unit instead. See
    equivalent systemd methods for details.

  * `EnableUnitFiles(in  as files, in  b runtime, in  b force, out b carries_install_info, out a(sss) changes);`

    `EnableUnitFiles()` may be used to enable one or more units in the system (by creating symlinks to them in /etc/ or /run/).
    It takes a list of unit files to enable
    (either just file names or full absolute paths if the unit files are residing outside the usual unit search paths)
    and two booleans: the first controls whether the unit shall be enabled for runtime only (true, /run/), or persistently (false, /etc/).
    The second one controls whether symlinks pointing to other units shall be replaced if necessary.
    This method returns one boolean and an array of the changes made.
    The boolean signals whether the unit files contained any enablement information (i.e. an [Install] section).
    The changes array consists of structures with three strings: the type of the change (one of "symlink" or "unlink"),
    the file name of the symlink and the destination of the symlink.

  * `DisableUnitFiles(in  as files, in  b runtime, out a(sss) changes);`

    `DisableUnitFiles()` is similar to `EnableUnitFiles()` but
    disables the specified units by removing all symlinks to them in /etc/ and /run/

  * `GetUnitProperties(in s name, in s interface, out a{sv} props)`

    Returns the current properties for a named unit on the node. The returned properties are the same as you would
    get in the systemd properties apis.

  * `GetUnitProperty(in s name, in s interface, in property, out v value)`

    Get one named property, otherwise similar to GetUnitProperties

  * `SetUnitProperties(in s name, in b runtime, in a(sv) keyvalues)`

    Set named properties. If runtime is true the property changes do not persist across
    reboots.

  * `ListUnits(out a(ssssssouso) units)`

    Returns all the currently loaded systemd units on this node. The returned structure is the same as the one returned
    by the systemd `ListUnits()` call.

  * `Reload()`

    `Reload()` may be invoked to reload all unit files.

  * `SetLogLevel(in s log_level)`

    Set the new log level for bluechi-agent by invoking the internal bluechi-agent API.

#### Properties

  * `Name` - `s`

    The name of the node

  * `Status` - `s`

    Status of the node, currently one of: `online`, `offline`. Emits changed when this changes.
  
  * `PeerIp` - `s`

    IP of the node.
    The address might be set even though the node is still offline since a call to
    org.eclipse.bluechi.Controller.Register hasn't been made.

  * `LastSeenTimestamp` - `t`

    Timestamp of the last successfully received heartbeat of the node.

### interface org.eclipse.bluechi.Job

Each potentially long-running operation returns a job object, which can be used to monitor the status of the job as well
as cancelling it.

Object path: `/org/eclipse/bluechi/job/$id`

#### Methods

  * `Cancel()`

    Cancel the job, which means either cancelling the corresponding systemd job if it was started, or directly
    cancelling or replacing the BlueChi job if no systemd job was started for it yet (i.e. it is in the queue).

#### Properties

  * `Id` - `u`

    An integer giving the id of the job

  * `Node` - `s`

    The name of the node the job is on

  * `Unit` - `s`

    The name of the unit the job works on

  * `JobType` - `s`

    Type of the job, either `Start` or `Stop`.

  * `State` - `s`

    The current state of the job, one of: `waiting` or `running`. Waiting is for queued jobs.

## BlueChi-Agent public D-Bus API

The main entry point is at the `/org/eclipse/bluechi` object path and implements the `org.eclipse.bluechi.Agent`
interface.

### interface org.eclipse.bluechi.Agent

#### Methods

  * `CreateProxy(in s service_name, in s node_name, in s unit_name)`

    Whenever a service on the agent requires a service on another node it creates a proxy service and calls this method.
    It then creates a new `org.eclipse.bluechi.internal.Proxy` object and emits the `ProxyNew` signal on the internal bus to
    tell the controller about it. The controller will then try to arrange that the requested unit on the specified node is
    running and notifies the initial agent about the status by calling `Ready` on the internal bus.

  * `RemoveProxy(in s service_name, in s node_name, in s unit_name)`

    When a proxy is not needed anymore it is being removed on the node and a `ProxyRemoved` is emitted to notify the controller.

  * `SwitchController(in s controller_address)`

    Set the new controller address for bluechi-agent node and trigger a reconnect to the controller with the new address.

### interface org.eclipse.bluechi.Metrics

This interface provides signals for collecting metrics. It is created by calling `EnableMetrics` on the `org.eclipse.bluechi.Controller` interface and removed by calling `DisableMetrics`.

#### Signals

  * `StartUnitJobMetrics(s node_name, s job_id, s unit, t job_measured_time_micros, t unit_start_prop_time_micros)`

    `StartUnitJobMetrics` will be emitted when a `Start` operation processed by BlueChi finishes and the collection of metrics has been enabled previously.

  * `AgentJobMetrics(s node_name, s unit, s method, t systemd_job_time_micros)`

    `AgentJobMetrics` will be emitted for all unit lifecycle operations (e.g. `Start`, `Stop`, `Reload`, etc.) processed by BlueChi when these finish and the collection of metrics has been enabled previously.

## Internal D-Bus APIs

The above APIs are the public facing ones that users of BlueChi would use. Additionally there are additional APIs that are
used internally to synchronize between the controller and the nodes, and sometimes internally on a node. We here describe
these APIs.

### interface org.eclipse.bluechi.internal.Controller

When a node connects to the controller it does so not via the public API, but via a direct peer-to-peer connection. On this
connection the regular Controller API is not available, instead we're using internal Controller object as the basic data.

Object path: `/org/eclipse/bluechi/internal`

#### Methods

  * `Register(in st name)`

    Before anything else can happen the node must call this method to register with the controller, giving its unique name.
    If this succeeds, then the controller will consider the node online and start forwarding operations to it.

#### Signals

  * `Heartbeat()`

    This is a periodic signal from the controller to the agent.

### interface org.eclipse.bluechi.internal.Agent

This is the main interface that the node implements and that is used by the controller to affect change on the node.

#### Methods

  * `StartUnit(in s name, in s mode, in u id)`

  * `StopUnit(in s name, in s mode, in u id)`

  * `ReloadUnit(in s name, in s mode, in u id)`

  * `RestartUnit(in s name, in s mode, in u id)`

  * `GetUnitProperties(in name, out a{sv} props)`

  * `ListUnits(out a(ssssssouso) units);`

    These are all API mirrors of the respective method in `org.eclipse.bluechi.Node`, and all they do is forward the
    same operation to the local systemd instance. Similarly, any changes in the systemd job will be forwarded to signals
    on the node job which will then be forwarded to the controller job and reach the user.

  * `Subscribe(in unit s)`

    Whenever some monitor object exists in the controller that matches a specific the node name and unit name, this method
    is called. This can happen either when a monitor is created or when a new node connects. Whenever *some*
    subscription is active, the node will call the systemd `Subscribed` method, and then register for `UnitNew`,
    `UnitRemoved` as well as for property change events on units. Any time a Unit changes it will emit the
    `UnitPropertyChanged` event if any of the supported properties changed since last time.

  * `Unsubscribe(in unit s)`

    Remove a subscription added via `Subscribe()`. If there are none left, call `Unsubscribe()` in the systemd API.

  * `EnableMetrics()`

    Enables the collection of metrics on this agent.

  * `DisableMetrics()`

    Disables the collection of metrics on this agent.

  * `StartDep(in unit s)`

    Starts a dependency systemd service on the specified unit. This is used by the proxy service to ensure that required systemd services on other nodes are running.

  * `StopDep(in unit s)`

    Stops a dependency systemd service on the specified unit.

  * `Reload()`

    `Reload` causes systemd on the agent to reload all unit files.

  * `SetLogLevel(in s log_level)`

    Set the new log level for bluechi-agent node. This change is persistent as long as bluechi-agent not restarted.

#### Signals

  * `JobDone(u id, s result)`

    Mirrors of the job signals in the controller and used to forward state changes from systemd to the controller.

  * `JobStateChanged(u id, s state)`

    Forwards the job state property changes from systemd to the controller.

  * `UnitPropertiesChanged(s unit, s interface, a{sv} props)`

    This is equivalent to the Monitor signal, but for this node only. The same set of properties described there is
    supported here.

  * `ProxyNew(s node_name, s unit_name, o proxy)`

    Whenever a proxy service is running on the system with the node it calls into the node service, and the node service
    creates a new `org.eclipse.bluechi.internal.Proxy` object and emits this signal to tell the controller about it. The controller
    will notice this and try to arrange that the requested unit is running on the requested node. If the unit is already
    running, when it is started, or when the start fails, the controller will call the `Ready()` method on it.

  * `ProxyRemoved(s node_name, s unit_name)`

    This is emitted when a proxy is not needed anymore because the service requiring the proxy
    service is stopped.

  * `Heartbeat()`

    This is a periodic signal from the node to the controller.

### interface org.eclipse.bluechi.internal.Proxy

The node creates one of these by request from a proxy service, it is used to synchronize the state of the remote service
with the proxy.

#### Methods

  * `Ready(in s result)`

    Called by the controller when the corresponding service is active (either was running already, or was started), or when
    it failed. result is `done` if it was already running, otherwise it is the same value as the remote node returned in
    result from its start job.

### interface org.eclipse.bluechi.internal.Agent.Metrics

This is the interface that provides signals sent from the agent to the controller to collect metrics, e.g. time measurements.

#### Signals

  * `AgentJobMetrics(s unit, s method, t systemd_job_time_micros)`

    This is emitted for each completed job when the collection of metrics has been enabled via `EnableMetrics`.
