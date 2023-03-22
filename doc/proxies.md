# Proxy Services

## Using Proxy Services

In order to support cross-node systemd unit dependencies, hirte
introduces something called proxy services. The hirta agent
comes with a template service called `hirte-proxy@.service`
which is the core of this mechanism.

Suppose there is a regular service file on node 'foo', which
looks looks like this

**`need-db.service`**

``` INI
[Unit]
Wants=db.service
After=db.service

[Service]
ExecStart=/bin/db-user
```

When this service is started, `db.service` is also started. But,
suppose you want to start `db.service` on another node `bar`.
With hirte you can do this like this:

**`need-db.service`**

``` INI
[Unit]
Wants=hirte-proxy@bar_db.service
After=hirte-proxy@bar_db.service

[Service]
ExecStart=/bin/db-user
```

When the `hirte-proxy@bar_db.service` is started it will talk to hirte
and initiate the process of starting `db.service` (if needed) on node
`bar`, and when it is becomes active (or it is detected it was already
active), the proxy service also becomes active.

If there is any problem activating the service, then the proxy service
will fail to activate. With the `Wants=` options used in the example
above this doesn't do anything, but you can use stronger dependencies
like `Requires=` which will make the activation of `need-db.service`
not start if the dependency fails to start.

Note: For startup performance and robustness it is generally better to
use weaker dependencies and handle failuress in other ways, like
service restarts.

After a successful start, the proxy service will continue to be active
as long as some other service depends on it, until the target service
on the other node stops. When hirte detects that the target service
becomes inactive, the proxy service will be stopped. This can be used
with even stronger dependency options like `BindTo=` to cause the
need-db service to stop when the db service stops.

In addition, when the last dependency of the proxy server on a node
exits, the proxy service will stop too (as it has
`StopWhenUnneeded=yes`), and hirte will propagate this information to
the target node. By default, in systemd this doesn't do anything, even
if there are no other dependencies on the target service. However you
can use `StopWhenUnneeded=yes` in the service to make it stop when
the last dependency (local or via proxy) stops.

## Internal Details on the source node

The proxy service is a template service called `hirte-proxy@.service`
of type `oneshoot` with `RemainAfterExit=yes`. This means that when it
is started it will change into `activating` stage, and then start the
`ExecStart` command. If this fails, it will go to `failed` state, but
when it eventually succeeds it will go into `active` state (even though
no process is running).

The `ExecStart` command starts the hirte-proxy helper app that talks
to the local agent, which in turn talks to the main hirte service,
starting the target service. Once it is running hirte notifies the
agent which in turns replies to the hirte-proxy which then exits with
the correct (failed or activated) exit status.

The proxy can be stopped on the local system (explicitly, or when the
last dependency to is stops), which will trigger the `ExecPost` command,
which tells the agent to unregister the proxy with hirte, which in turn
stops the dependency on the target service on the target node.

Alternatively, if hirte notices that the target service stopped, after
we returned successfully in the `ExecStart` command, then the agent
explicitly stops the proxy service (via systemd).

## Internal Details on the target node

The hirte agent also contains another template service called
`hirte-dep@.service` which is used on the target node. This service is
templated on the target service name, such that whenever
`hirte-dep@XXX.service` it depends on `XXX.service` causing it to
start. Whenever there is a proxy service running on some other node,
hirte starts a dep service like this to mirror it, which makes systemd
consider the target needed.

The dependency used for the dep service is `BindsTo` and `After`,
which is a very strong dependency. This means the state of the dep
service mirrors the target service, i.e. stops when it stops. This is
done to handle the case where the target service stops for some
reason, and then there is a *new* proxy started. When this happens we
will again start the dep service, but if it wasn't stopped with the
target service it would already be running, and thus not trigger
a re-start of the target service.

## Implementation Details

Tracking service state across multiple nodes is very tricky, as the
state can diverge due to disconnects, delays, or races. To minimize
such problems most state and intelligence is kept in the
agent. Whenever the agent registers a new proxy it will announce this
to the manager (if connected), and this will start a one-directional
flow of non-interpreted state-change events from the target service to
the manager to the agent, until the agent explicitly removes the
proxy.

If an agent is disconnected from the manager, then the manager treats
that as if the agent removed the proxy. On re-connection the agent
will re-send the registering of the proxy.

In addition to the monitoring, each time a proxy is registered the
manager will tell the target node to start the dep service for the
target service. Hirte keeps track of how many proxies are outstanding
for the target service and only tell the agent to stop the dep service
when this reaches zero. Similar to the above, when the target node
reconnect we re-sent starts for any outstanding proxies.

Note that due to disconnects we may sometimes send multiple start
events to the target service, and we may report virtual "stop" events
of the target when its really still active, only disconnected.
