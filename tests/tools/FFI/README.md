<!-- markdownlint-disable-file MD013 MD040 MD041 -->

This is **BlueChi Tester**, and its purpose is to create tests that can disrupt or diminish the functionality of the BlueChi controller using DBus interface.

Prior to employing the BlueChi Tester, ensure that the BlueChi controller contains a specific node name for the intended test in the AllowedNodeNames list.

In the controller, utilize journald or stderr to keep track of the actions performed by this tool.

Example of usage:

``` bash
./bluechi-tester \
    --url="tcp:host=10.90.0.2,port=842" \
    --nodename=control \
    --signal="JobDone" \
    --numbersignals=500
```
