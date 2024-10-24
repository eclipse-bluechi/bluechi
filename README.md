<!-- markdownlint-disable-file MD041 -->
![BlueChi logo](https://raw.githubusercontent.com/eclipse-bluechi/bluechi/main/logo/bluechi-logo-horiz.png)

# Eclipse BlueChi&trade; : A systemd service controller for multi-node environments

[![Copr build status](https://copr.fedorainfracloud.org/coprs/g/centos-automotive-sig/bluechi-snapshot/package/bluechi/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/g/centos-automotive-sig/bluechi-snapshot/package/bluechi/)
[![Integration Tests](https://github.com/eclipse-bluechi/bluechi/actions/workflows/integration-tests.yml/badge.svg?branch=main)](https://github.com/eclipse-bluechi/bluechi/actions/workflows/integration-tests.yml)
[![Unit Tests](https://github.com/eclipse-bluechi/bluechi/actions/workflows/unit-tests.yml/badge.svg?branch=main)](https://github.com/eclipse-bluechi/bluechi/actions/workflows/unit-tests.yml)
[![Coverage Status](https://coveralls.io/repos/github/eclipse-bluechi/bluechi/badge.svg?branch=main)](https://coveralls.io/github/eclipse-bluechi/bluechi?branch=main)

Eclipse BlueChi&trade; is a systemd service controller intended for
multi-node environments with a predefined number of nodes and with a focus on
highly regulated ecosystems such as those requiring functional safety.
Potential use cases can be found in domains such as transportation, where
services need to be controlled across different edge devices and where
traditional orchestration tools are not compliant with regulatory requirements.

BlueChi is relying on [systemd](https://github.com/systemd/systemd) and its D-Bus
API, which it extends for multi-node use case.

BlueChi can also be used to control systemd services for containerized applications
using [Podman](https://github.com/containers/podman/) and its ability
to generate systemd service configuration to run a container via
[quadlet](https://www.redhat.com/sysadmin/quadlet-podman).

## How to contribute

Please refer to our [contribution guideline](./CONTRIBUTING.md).
