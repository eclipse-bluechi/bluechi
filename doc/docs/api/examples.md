<!-- markdownlint-disable-file MD013 MD024 -->
# Examples using the public D-Bus API

## GoLang

First, make sure the system has golang installed and download the dbus library.

These steps were based on CentOS Stream 9:

### Installing golang

``` bash
# dnf install go-toolset
```

### Adding dbus library

``` bash
# go install github.com/godbus/dbus/v5
```

### Build the source code

``` bash
# go mod init list_nodes
# go mod tidy 
# go run list_nodes.go
# ./list_nodes
Name: control
Status: online
Object Path: /org/eclipse/bluechi/node/control

Name: node1
Status: online
Object Path: /org/eclipse/bluechi/node/node1

Name: qm-node1
Status: online
Object Path: /org/eclipse/bluechi/node/qm_2dnode1
```

### List all nodes

``` go
--8<-- "api-examples/go/list_nodes.go"
```

---

## Python

The examples listed here are using the [dasbus](https://dasbus.readthedocs.io/en/latest/) library for Python 3. It can be installed via

```bash
pip3 install dasbus
```

---

### List all nodes

```python
--8<-- "api-examples/list-nodes.py"
```

---

### List all units on a given node

```python
--8<-- "api-examples/list-node-units.py"
```

---

### List active services on all nodes

```python
--8<-- "api-examples/list-active-services.py"
```

---

### Starting a unit

```python
--8<-- "api-examples/start-unit.py"
```

---

### Monitor a unit

```python
--8<-- "api-examples/monitor-unit.py"
```

---

### Get a unit property

```python
--8<-- "api-examples/get-unit-property.py"
```

---

### Set a unit property

```python
--8<-- "api-examples/set-cpuweight.py"
```

---

### Enable unit files

```python
--8<-- "api-examples/enable-unit.py"
```

---
