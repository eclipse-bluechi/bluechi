<!-- markdownlint-disable-file MD013 -->
# BlueChi integration test

## Installation

The integration tests use the RESTful API of [podman](https://podman.io/getting-started/installation) to isolate BlueChi
and the agents on multiple, containerized nodes. Therefore, a working installation of podman is required. Please refer
to [podman installation instructions](https://podman.io/getting-started/installation).

### Installing packages using RPM

First, enable required repositories on CentOS Stream 9:

```shell
sudo dnf install -y dnf-plugin-config-manager
sudo dnf config-manager -y --set-enabled crb
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

**_NOTE:_** Integration tests code should be compatible with Python 3.9, please don't use features from newer versions.

### Installing packages using pip

All required python packages are listed in the [requirements.txt](./requirements.txt) and can be installed using `pip`:

```bash
pip install -U -r requirements.txt
```

Instead of installing the required packages directly, it is recommended to create a virtual environment. For example,
the following snippet uses the built-in [venv](https://docs.python.org/3/library/venv.html):

```bash
python -m venv ~/bluechi-env
source ~/bluechi-env/bin/activate
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

This will use latest BlueChi packages from
[hirte-snapshot](https://copr.fedorainfracloud.org/coprs/mperina/hirte-snapshot/) repository.

## Running integration tests with local BlueChi build

In order to run integration tests for your local BlueChi build, you need have BlueChi RPM packages built from your source
code. The details about BlueChi development can be found at
[README.developer.md](https://github.com/containers/bluechi/blob/main/README.developer.md), the most important part for
running integration tests is [Packaging](https://github.com/containers/bluechi/blob/main/README.developer.md#packaging)
section.

In the following steps BlueChi source codes are located in `~/bluechi` directory.

Integration tests are expecting, that local BlueChi RPMs are located in `tests/bluechi-rpms` top level subdirectory, so BlueChi
packages should be built using following commands:

```shell
mkdir ~/bluechi/tests/bluechi-rpms
cd ~/bluechi
sudo ARTIFACTS_DIR=~/bluechi/tests/bluechi-rpms ./build-scripts/build-rpm.sh
```

When done it's required to create DNF repository from those RPMs:

```shell
createrepo_c ~/bluechi/tests/bluechi-rpms
```

After that step integration tests can be executed using following command:

```shell
cd ~/bluechi/tests
tmt run -eCONTAINER_USED=integration-test-local
```

## Developing integration tests

### Code Style

[autopep8](https://pypi.org/project/autopep8/) is used to enforce a unified code style. The applied rules are defined in
the [.pep8](./.pep8) file. All source files can be formatted via:

```bash
# navigate into bluechi/tests directory
autopep8 ./
```

### Using python bindings in tests

The [python bindings](../src/bindings/python/) can be used in the integration tests to simplify writing them. However, it is not possible to use the bindings directly in the tests since they are leveraging the D-Bus API of BlueChi provided on the system D-Bus. A separate script has to be written, injected and executed within the container running the BlueChi controller. In order to keep the usage rather simple, the `BluechiControllerContainer` class provides a function to abstract these details:

```python
# run the file ./python/monitor.py located in the current test directory
# and get the exit code as well as the output (e.g. all print())
exit_code, output = ctrl.run_python("python/monitor.py")
```

A full example of how to use the python bindings can be found in the [monitor open-close test](./tests/tier0/monitor-open-close/).

## Usage of containers

The integration tests rely on containers as separate compute entities. These containers are used to simulate BlueChi's
functional behavior on a single runner.

The [containers](./containers/) directory contains two container files.

The `integration-test-base` file describes the builder base image that is published to
[https://quay.io/repository/bluechi/integration-test-base](https://quay.io/repository/bluechi/integration-test-base). It contains core dependencies such as systemd and devel packages.

Both, `integration-test-local` as well as `integration-test-snapshot`, are based on the builder base image for the integration tests and contain compiled products and configurations for integration testing.

### Updating container images in registry

Currently, the base images are pushed manually to the [bluechi on quay.io](https://quay.io/organization/bluechi) organization and its repositories. If any updates are required, please reach out to the [code owners](../.github/CODEOWNERS).

**Note to codeowners:**

The base images [build-base](./containers/build-base) and [integration-test-base](./containers/integration-test-base) can be built for multiple platforms (arm64 and amd64) using the [build-containers.sh](../build-scripts/build-containers.sh) script. It'll build images for the given platforms as well as a manifest with the same name, which can then be pushed to the registry. From the root directory of the project run the following commands:

```bash
# building and publishing build-base image
bash build-scripts/build-containers.sh build_base
podman login -u="someuser" -p="topsecret" quay.io
podman manifest push quay.io/bluechi/build-base:latest docker://quay.io/bluechi/build-base:latest
```
