<!-- markdownlint-disable-file MD013 MD040 MD041 -->

This is a BlueChi Tester, the idea behind is try to make any kind of test to interfere or reduce the ability to work in the BlueChi controller.

Before using the bluechi-tester, make sure in the bluechi controller has a node name for this specific test in the AllowedNodeNames.
In the controller, use journald or stderr to monitor the actions this tools has being doing.

Example of usage:

``` bash
[root@node ~]# git clone https://github.com/containers/qm
[root@node qm]# cd qm/tests/e2e/tools/FFI/bluechi-tester

[root@node bluechi-tester]# go mod init bluechi
go: creating new go.mod: module bluechi
go: to add module requirements and sums:
	go mod tidy

[root@node bluechi-tester]# go mod tidy
go: finding module for package github.com/godbus/dbus/v5
go: downloading github.com/godbus/dbus v4.1.0+incompatible
go: downloading github.com/godbus/dbus/v5 v5.1.0
go: found github.com/godbus/dbus/v5 in github.com/godbus/dbus/v5 v5.1.0

[root@node bluechi-tester]# go build bluechi-tester.go

[root@node bluechi-tester]# ./bluechi-tester
Usage:
  bluechi-tester [flags]

Flags:
  -h, --help                help for bluechi-tester
  -n, --nodename string     Specify a node name
  -N, --numbersignals int   Number of signals to be sent (default 1)
  -s, --signal string       Specify a signal name
  -u, --url string          Specify a URL


Usage Example:

    ./bluechi-tester
        --url tcp:host=10.90.0.4,port=842
        --nodename NODE_AGENT_NAME
        --signal JobDone
        --numbersignals 500
```
