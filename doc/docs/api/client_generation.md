<!-- markdownlint-disable-file MD013 -->

# Generating BlueChi clients

BlueChi provides [introspection data](https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format) for its public API. These XML files are located in the [data directory](https://github.com/eclipse-bluechi/bluechi/tree/main/data) of the project and can be also be used as input to generate clients.

There is a variety of code generation tools for various programming languages. There is a list

- `C`: [gdbus-codegen](https://developer-old.gnome.org/gio/stable/gdbus-codegen.html)
- `Go`: [dbus-codegen-go](https://github.com/amenzhinsky/dbus-codegen-go)
- `Rust`: [dbus-codegen-rust](https://crates.io/crates/dbus-codegen)
- and many more

Using generated code for clients has the advantage of reducing boilerplate code as well as providing abstracted functions for the different interfaces and operations on them.

## Typed Python Client

For `python`, the dynamically generated proxies (e.g. from [dasbus](https://github.com/rhinstaller/dasbus)) don't require any generation and work well on their own. Because of that there are no larger projects to generate python code from a D-Bus specification. These proxies, however, don't provide (type) hints.

Therefore, the BlueChi project contains its own [generator](https://github.com/eclipse-bluechi/bluechi/tree/main/src/bindings/generator) to output typed python bindings based on introspection data.

### Installation

The auto-generated, typed python bindings are published as an RPM package as well as a PyPI package. So it can be installed via:

```bash
# install from rpm package
dnf install python3-bluechi

# or from PyPI
pip install bluechi
```

Or build and install it directly from source:

```bash
git clone git@github.com:eclipse-bluechi/bluechi.git
cd bluechi
pip install -r src/bindings/generator/requirements.txt

./build-scripts/generate-bindings.sh python
./build-scripts/build-bindings.sh python
pip install src/bindings/python/dist/bluechi-0.6.0-py3-none-any.whl
```

### Usage

The `bluechi` python package consists of two subpackages:

- `api`: auto-generated code based the BlueChi D-BUS API description
- `ext`: custom written code to simplify common tasks

Functions from the auto-generated `api` subpackage reduce boilerplate code. It also removes the need to explicitly specify the D-Bus name, paths and interfaces and offers abstracted and easy to use functions. For example, let's compare the python code for listing all nodes.

Using the auto-generated bindings, the code would look like this:

```python
--8<-- "bluechi-examples/ListAllNodes.py"
```

The functions from the `ext` subpackage leverage the functions in `api` and combine them to simplify recurring tasks. A good example here is to wait for a systemd job to finish. When starting a systemd unit via BlueChi (e.g. see [start unit example](./examples.md#start-unit)), it returns the path to the corresponding job for the start operation of the systemd unit. This means that the unit hasn't started yet, but is queued to be. BlueChi will emit a signal when the job has been completed. Starting a unit and waiting for this signal has been encapsulated by the `ext.Unit` class:

```python
--8<-- "bluechi-examples/StartUnit.py"
```

If there is a common task for which `bluechi` can provide a simplifying function, please submit an [new RFE request](https://github.com/eclipse-bluechi/bluechi/issues/new/choose).

For more examples, please have a look at the [next section](#more-examples)

### More examples

The following code snippets showcase more examples on how `bluechi` can be used.

#### Stop a unit

```python
--8<-- "bluechi-examples/StopUnit.py"
```

#### Enable a unit

```python
--8<-- "bluechi-examples/EnableUnit.py"
```

#### Disable a unit

```python
--8<-- "bluechi-examples/DisableUnit.py"
```

#### List all units on all nodes

```python
--8<-- "bluechi-examples/ListNodeUnits.py"
```

#### List all active services on all nodes

```python
--8<-- "bluechi-examples/ListActiveServices.py"
```

#### Get a unit property

```python
--8<-- "bluechi-examples/CPUWeight.py"
```

#### Set a unit property

```python
--8<-- "bluechi-examples/SetCPUWeight.py"
```

#### Monitor the connections of all nodes

```python
--8<-- "bluechi-examples/MonitorNodeConnections.py"
```

#### Monitor the connection on the managed node

```python
--8<-- "bluechi-examples/MonitorAgentConnection.py"
```

#### Monitor system status

```python
--8<-- "bluechi-examples/MonitorSystemStatus.py"
```
