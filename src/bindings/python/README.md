# hirte python bindings

The hirte python bindings provides a python module to interact with the D-Bus API of hirte. It consists of the following
subpackages:

- `api`: auto-generated code based the hirte D-BUS API description
- `ext`: custom written code to simplify common tasks

## Installation

Using `pip3`:

```sh
# from PyPi
pip3 install pyhirte
# or from cloned git repo
pip3 install --force dist/pyhirte-<version>-py3-none-any.whl 
```

## Examples

Listing all connected nodes and their current state:

```python
from pyhirte.api import Manager

for node in Manager().list_nodes():
    # node[name, obj_path, status]
    print(f"Node: {node[0]}, State: {node[3]}")
```

Starting and stopping of a systemd unit on a specific node using the `Unit` class from the `ext` subpackage to
implicitly wait for the job to finish:

```python
from pyhirte.ext import Unit

hu = Unit("my-node-name")

result = hu.start_unit("chronyd.service")
print(result)

result = hu.stop_unit("chronyd.service")
print(result)
```
