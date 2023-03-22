# Hirte

[![Copr build status](https://copr.fedorainfracloud.org/coprs/mperina/hirte-snapshot/package/hirte/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/mperina/hirte-snapshot/package/hirte/)

Hirte is a systemd service controller intended for multi-node environments with
a predefined number of nodes and with a focus on highly regulated ecosystems
such as those requiring functional safety.
Potential use cases can be found in domains such as transportation, where
services need to be controlled across different edge devices and where
traditional orchestration tools are not compliant with regulatory requirements.

Hirte is relying on [systemd](https://github.com/systemd/systemd) and its D-Bus
API, which it extends for multi-node use case.

Hirte can also be used to control systemd services for containerized applications
using [Podman](https://github.com/containers/podman/) and its ability
to generate systemd service configuration to run a container via
[quadlet](https://www.redhat.com/sysadmin/quadlet-podman).

## How to contribute

### Testing

RPM packages for the Hirte project are available on
[hirte-snapshot](https://copr.fedorainfracloud.org/coprs/mperina/hirte-snapshot/)
COPR repo. To install hirte packages on your system please add that repo using:

```bash
dnf copr enable mperina/hirte-snapshot
```

When done you can install relevant hirte packages using:

```bash
dnf install hirte hirte-agent
```

### Submitting patches

Patches are welcome!

Please submit patches to [github.com/containers/hirte](https://github.com/containers/hirte).
More information about the development can be found in [README.developer.md](README.developer.md).

You can read [Get started with GitHub](https://docs.github.com/en/get-started)
if you are not familiar with the development process to learn more about it.

### Found a bug or documentation issue?

To submit a bug or suggest an enhancement please use [GitHub issues](https://github.com/containers/hirte/issues).

## Still need help?

Please join the [CentOS Automotive SIG mailing list](https://lists.centos.org/mailman/listinfo/centos-automotive-sig/)
if you have any other questions.
