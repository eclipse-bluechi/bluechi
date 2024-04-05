<!-- markdownlint-disable-file MD013 -->
# Using BlueChi's D-Bus API

BlueChi provides [introspection data](https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format) for its public API. These XML files are located in the [data directory](https://github.com/eclipse-bluechi/bluechi/tree/main/data) of the BlueChi project and can be used either as reference when writing custom clients or as input to generate them (see [here](./client_generation.md)).

There are a number of bindings for the D-Bus protocol for a range of programming languages. A (not complete) list of bindings can be found on the [freedesktop D-Bus bindings](https://www.freedesktop.org/wiki/Software/DBusBindings/) website.

The following sections demonstrate how the D-Bus API of BlueChi can be interacted with using bindings from different programming languages by showing small snippets.

!!! note

    All snippets require to be run on the machine where BlueChi is running. The only exception is the example for [monitoring the connection status of the agent](#monitor-the-connection-on-the-managed-node), which has to be executed on the managed node where the BlueChi agent is running.

## Setup

=== "Go"

    --8<-- "api-examples/go/setup.md"

=== "Python"

    --8<-- "api-examples/python/setup.md"

=== "Rust"

    --8<-- "api-examples/rust/setup.md"

!!! note

    Depending on the setup of BlueChi root privileges might be needed when running the samples.

## Getting information

### List all nodes

=== "Go"

    ```go
    --8<-- "api-examples/go/list-nodes.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/list-nodes.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/list-nodes.rs"
    ```

### List all units on a node

=== "Go"

    ```go
    --8<-- "api-examples/go/list-node-units.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/list-node-units.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/list-node-units.rs"
    ```

### Get a unit property value

=== "Go"

    ```go
    --8<-- "api-examples/go/get-cpuweight.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/get-cpuweight.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/get-cpuweight.rs"
    ```

## Operations on units

### Start unit

=== "Go"

    ```go
    --8<-- "api-examples/go/start-unit.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/start-unit.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/start-unit.rs"
    ```

### Enable unit

=== "Go"

    ```go
    --8<-- "api-examples/go/enable-unit.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/enable-unit.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/enable-unit.rs"
    ```

### Set property of a unit

=== "Go"

    ```go
    --8<-- "api-examples/go/set-cpuweight.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/set-cpuweight.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/set-cpuweight.rs"
    ```

## Monitoring

### Monitor the connections of all nodes

=== "Go"

    ```go
    --8<-- "api-examples/go/monitor-node-connections.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/monitor-node-connections.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/monitor-node-connections.rs"
    ```

### Monitor the connection on the managed node

=== "Go"

    ```go
    --8<-- "api-examples/go/monitor-agent-connection.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/monitor-agent-connection.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/monitor-agent-connection.rs"
    ```

### Monitor unit changes

=== "Go"

    ```go
    --8<-- "api-examples/go/monitor-unit.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/monitor-unit.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/monitor-unit.rs"
    ```

### Monitor system status

=== "Go"

    ```go
    --8<-- "api-examples/go/monitor-system-status.go"
    ```

=== "Python"

    ```python
    --8<-- "api-examples/python/monitor-system-status.py"
    ```

=== "Rust"

    ```rust
    --8<-- "api-examples/rust/monitor-system-status.rs"
    ```
