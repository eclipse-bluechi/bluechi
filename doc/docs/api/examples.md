<!-- markdownlint-disable-file MD013 -->
# Examples using the public D-Bus API

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
