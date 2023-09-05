<!-- markdownlint-disable-file MD013 MD024 -->
# Examples using the public D-Bus API

## Python

The examples listed here are using the [dasbus](https://dasbus.readthedocs.io/en/latest/) library for Python 3. It can be installed via

```bash
pip3 install dasbus
```

---

### List all nodes

```python
--8<-- "api-examples/python/list-nodes.py"
```

---

### List all units on a given node

```python
--8<-- "api-examples/python/list-node-units.py"
```

---

### List active services on all nodes

```python
--8<-- "api-examples/python/list-active-services.py"
```

---

### Starting a unit

```python
--8<-- "api-examples/python/start-unit.py"
```

---

### Monitor a unit

```python
--8<-- "api-examples/python/monitor-unit.py"
```

---

### Get a unit property

```python
--8<-- "api-examples/python/get-unit-property.py"
```

---

### Set a unit property

```python
--8<-- "api-examples/python/set-cpuweight.py"
```

---

### Enable unit files

```python
--8<-- "api-examples/python/enable-unit.py"
```

---

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

## Rust

Rust developers can take advance of **dbus-codegen-rust** to generate a Rust module
and use,tag,release in any pace it's appropriate to their project.

### Generating bluechi module

Make sure a BlueChi node is running and execute the below commands.

``` bash
# dnf install rust cargo
# cargo install dbus-codegen
# dbus-codegen-rust -s -g -m None -d org.eclipse.bluechi -p /org/eclipse/bluechi > bluechi.rs
```

### Using the generated code

As every project is different, here a traditional example of importing the generated module in a **main.rs**:

**Dir structure**:

``` bash
.
├── Cargo.toml
└── src
    ├── bluechi.rs
    └── main.rs
```

As soon the files **bluechi.rs** and [main.rs](./src/main.rs) were copies to `src/` it's possible to compile a new binary via:

``` bash
# cargo build
# ./list_nodes
Node: "control"
Status: "online"
Path("/org/eclipse/bluechi/node/control\0")
Node: "node1"
Status: "offline"
Path("/org/eclipse/bluechi/node/node1\0")
```

### List all nodes

``` rust
--8<-- "api-examples/rust/list_nodes.rs"
```
