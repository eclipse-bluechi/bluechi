# golang examples

First, make sure the system has golang installed and download the dbus library.  

These steps were based on CentOS Stream 9:

**Installing go**:

``` bash
# dnf install go-toolset
```

**Adding dbus library**:

``` bash
# go install github.com/godbus/dbus
```

Build the source code:

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
