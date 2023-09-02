<!-- markdownlint-disable-file MD013 MD024 -->
# Examples using the public D-Bus API

These steps were based on CentOS Stream 9

## Ruby

### Installing ruby and dbus module

``` bash
dnf install ruby -y
gem install ruby-dbus
```

### Build the source code

``` bash
# ruby list_nodes.rb
BlueChi nodes:
================
Name: control
Path: online
Status: /org/eclipse/bluechi/node/control

Name: node1
Path: offline
Status: /org/eclipse/bluechi/node/node1:x
```

### List all nodes

``` ruby
--8<-- "api-examples/ruby/list_nodes.rb"
```

---

## GoLang

### Installing golang and dbus module

``` bash
# dnf install go-toolset
# go install github.com/godbus/dbus
```

### Build the source code

``` bash
# go build list_nodes.go
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
