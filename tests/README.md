# Hirte integration test

## Installation

## Enable required repositories on CentOS Stream 9

Following repositories are required on CentOS Stream 9 for successful installation:

```shell
dnf install -y dnf-plugin-config-manager
dnf config-manager -y --set-enabled crb
dnf install -y epel-release
```

## Install packages

Following packages need to be installed for integration tests execution:

```shell
dnf install \
    createrepo_c \
    podman \
    python3-podman \
    python3-pytest \
    tmt \
    tmt-report-junit \
    -y
```

## Configure podman socket access for users

Testing infrastructure uses socket access to podman, so it needs to be enabled:

```shell
systemctl --user enable podman.socket
systemctl --user start podman.socket
```

## Running integration tests

Integration tests are executed with [tmt framework](https://github.com/teemtee/tmt).

To run integration tests please execute below command of the top level directory of this project:

```shell
tmt run
```

This will use latest hirte packages from
[hirte-snapshot](https://copr.fedorainfracloud.org/coprs/mperina/hirte-snapshot/) repository.

## Running integration tests with local hirte build

In order to run integration tests for your local hirte build, you need have hirte RPM packages built from your source
code. The details about hirte development can be found at
[README.developer.md](https://github.com/containers/hirte/blob/main/README.developer.md), the most important part for
running integration tests is [Packaging](https://github.com/containers/hirte/blob/main/README.developer.md#packaging)
section.

In the following steps hirte source codes are located in `~/hirte` directory.

Integration tests are expecting, that local hirte RPMs are located in `tests/hirte-rpms` top level subdirectory, so hirte
packages should be built using following commands:

```shell
mkdir ~/hirte/tests/hirte-rpms
cd ~/hirte
sudo ARTIFACTS_DIR=~/hirte/tests/hirte-rpms ./build-scripts/build-rpm.sh
```

When done it's required to create DNF repository from those RPMs:

```shell
createrepo_c ~/hirte/tests/hirte-rpms
```

After that step integration tests can be executed using following command:

```shell
cd ~/hirte/tests
tmt run -eCONTAINER_USED=container-hirte-local
```
