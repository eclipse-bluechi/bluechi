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

## Running integration tests with memory leak detection

To run integration tests with `valgrind`, set `WITH_VALGRIND` environment variable as follows:

```shell
WITH_VALGRIND=1 tmt run -v
```

If `valgrind` detects a memory leak in a test, the test will fail, and the logs will be found in the test `data` directory.

## Running integration tests with local BlueChi build

In order to run integration tests for your local BlueChi build, you need have BlueChi RPM packages built from your source
code. The details about BlueChi development can be found at
[README.developer.md](https://github.com/containers/bluechi/blob/main/README.developer.md), the most important part for
running integration tests is [Packaging](https://github.com/containers/bluechi/blob/main/README.developer.md#packaging)
section.

In the following steps BlueChi source codes are located in `~/bluechi` directory.

The integration tests expect that local BlueChi RPMs are located in `tests/bluechi-rpms` top level subdirectory.
In addition, since the tests run in CentOS-Stream9 based containers the RPMs must also be built for CentOS-Stream9.
To this end, a containerized build infrastructure is available.

The containerized build infrastructure depends on [skipper](https://github.com/Stratoscale/skipper),
installed via the [requirements.txt](./requirements.txt) file

```shell
cd ~/bluechi
skipper make rpm
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

### Changing log level

By default BlueChi integration tests are using `INFO` log level to display important information about the test run.
More detailed information can be displayed by setting log level to `DEBUG`:

```shell
cd ~/bluechi/tests
tmt run -eLOG_LEVEL=DEBUG
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

The base images [build-base](./containers/build-base) and [integration-test-base](./containers/integration-test-base) can be built for multiple architectures (arm64 and amd64) using the [build-containers.sh](../build-scripts/build-containers.sh) script. It'll build the images for the supported architectures as well as a manifest, which can then be pushed to the registry.

Building for multiple architectures, the following packages are required:

```bash
sudo dnf install -y podman buildah qemu-user-static
```

From the root directory of the project run the following commands:

```bash
# In order to build and directly push, login first
buildah login -u="someuser" -p="topsecret" quay.io
PUSH_MANIFEST=yes bash build-scripts/build-push-containers.sh build_base

# Only build locally
bash build-scripts/build-push-containers.sh build_base
```
