<!-- markdownlint-disable-file MD013 MD033 MD046 -->

# bluechi-is-online

The BlueChi project offers with `bluechi-is-online` a CLI tool for checking and monitoring the connection state of BlueChi components in the system. It can be wrapped in systemd units so that other units can be started or stopped based on the connection state.

!!! Note

    For more information please refer to the [man page of bluechi-is-online](../man/bluechi-is-online.md).

## Install bluechi-is-online on CentOS

```bash
sudo dnf install bluechi-is-online
```

## Verify Installation

You can verify that the CLI tool has been installed by:

```bash
$ bluechi-is-online help
bluechi-is-online checks and monitors the connection state of BlueChi components
...
```

## Usage example

=== "Stop service(s) when bluechi-agent loses connection"

    ```bash
    $ cat /etc/systemd/system/monitor-bluechi-agent.service
    [Unit]
    Description=Monitor bluechi-agents connection to controller

    [Service]
    Type=simple
    ExecStart=/usr/local/bin/bluechi-is-online agent --wait=2000 --switch-timeout=2000 --monitor

    $ cat /etc/systemd/system/workload.service
    [Unit]
    Description=Some workload that should stop running when bluechi-agent disconnects
    BindsTo=monitor-bluechi-agent.service
    After=monitor-bluechi-agent.service

    [Service]
    ...
    ```

=== "Start service(s) when bluechi-agent loses connection"

    ```bash
    $ cat /etc/systemd/system/handle-bluechi-agent-offline.service
    [Unit]
    Description=Handle BlueChi Agent going offline and start take-action.service
    OnFailure=take-action.service

    [Service]
    Type=simple
    ExecStart=/usr/local/bin/bluechi-is-online agent --wait=2000 --switch-timeout=2000 --monitor
    ```
