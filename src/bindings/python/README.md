# BlueChi Python bindings

The BlueChi Python bindings provides a Python module to interact with the D-Bus API of BlueChi. It consists of the
following subpackages:

- `api`: auto-generated code based the BlueChi D-BUS API description
- `ext`: custom written code to simplify common tasks

## Installation

Using `pip3`:

```sh
# from PyPi
pip3 install bluechi
# or from cloned git repo
pip3 install --force dist/bluechi-<version>-py3-none-any.whl
```

## Examples

Listing all connected nodes and their current state:

```python
from bluechi.api import Controller

for node in Controller().list_nodes():
    # node[name, obj_path, status]
    print(f"Node: {node[0]}, State: {node[3]}")
```

Starting and stopping of a systemd unit on a specific node using the `Unit` class from the `ext` subpackage to
implicitly wait for the job to finish:

```python
from bluechi.ext import Unit

hu = Unit("my-node-name")

result = hu.start_unit("chronyd.service")
print(result)

result = hu.stop_unit("chronyd.service")
print(result)
```
