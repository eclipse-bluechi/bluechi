<!-- markdownlint-disable-file MD010 MD013 MD014 MD024 MD046 -->
# Getting started with BlueChi

On Fedora and CentOS-Stream systems (with [EPEL](https://docs.fedoraproject.org/en-US/epel/) repository enabled), all components of BlueChi can be directly installed via:

```bash
dnf install bluechi \
    bluechi-agent \
    bluechi-ctl \
    bluechi-selinux \
    python3-bluechi
```

!!! Note

    The command above installs BlueChi's core components - `bluechi` and `bluechi-agent` - as well as additional packages. `bluechi-ctl` is a CLI tool to interact with BlueChi's public D-Bus API provided by `bluechi` (controller). An SELinux policy is installed via the `bluechi-selinux` package. The last one supplies the [typed python bindings](../api/client_generation.md#typed-python-client), which simplifies programming custom applications that interact with BlueChi's API. 
