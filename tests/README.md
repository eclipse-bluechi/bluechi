<!-- markdownlint-disable-file MD013 -->
# Hirte integration test

## Installation

The integration tests use the RESTful API of [podman](https://podman.io/getting-started/installation) to isolate hirte
and the agents on multiple, containerized nodes. Therefore, a working installation of podman is required. Please refer
to [podman installation instructions](https://podman.io/getting-started/installation).

### Installing packages using RPM

First, enable required repositories on CentOS Stream 9:

```shell
sudo dnf install -y dnf-plugin-config-manager
sudo dnf install -y --set-enabled crb
sudo dnf install -y epel-release
```

Then install the required packages:

```shell
dnf install \
    createrepo_c \
    podman \
    python3-podman \
    python3-pytest \
    python3-pytest-timeout \
    tmt \
    tmt-report-junit \
    -y
```

### Installing packages using pip

All required python packages are listed in the [requirements.txt](./requirements.txt) and can be installed using `pip`:

```bash
pip install -U -r requirements.txt
```

Instead of installing the required packages directly, it is recommended to create a virtual environment. For example,
the following snippet uses the built-in [venv](https://docs.python.org/3/library/venv.html):

```bash
python -m venv env
source env/bin/activate
pip install -U -r requirements.txt
# ...

# exit the virtual env
deactivate
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
tmt run -eCONTAINER_USED=integration-test-local
```

## Developing integration tests

### Code Style

[autopep8](https://pypi.org/project/autopep8/) is used to enforce a unified code style. The applied rules are defined in
the [.pep8](./.pep8) file. All source files can be formatted via:

```bash
# navigate into hirte/tests directory
autopep8 ./
```

## Usage of containers

The integration tests rely on containers as separate compute entities. These containers are used to simulate hirte's
functional behavior on a single runner.

The [containers](./containers/) directory contains two container files.

The `integration-test-base` file describes the builder base image that is published to
[https://quay.io/repository/hirte/integration-test-base](https://quay.io/repository/hirte/integration-test-base). It contains core dependencies such as systemd and devel packages.

Both, `integration-test-local` as well as `integration-test-snapshot`, are based on the builder base image for the integration tests and contain compiled products and configurations for integration testing.

**_NOTE:_** Currently, the images are pushed manually to the quay.io container repository. If any updates are required, please reach out to the [code owners](../.github/CODEOWNERS).
