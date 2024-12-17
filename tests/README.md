<!-- markdownlint-disable-file MD013 -->

# Eclipse BlueChi&trade; integration tests

## Installation

The integration tests use the RESTful API of [podman](https://podman.io/getting-started/installation) to isolate BlueChi
and the agents on multiple, containerized nodes. Therefore, a working installation of podman is required. Please refer
to [podman installation instructions](https://podman.io/getting-started/installation).

**_NOTE:_** Integration tests can be run on Python 3.9 and newer, but we want to have Python 3.9 compatibility to support CentOS Stream 9, so please don't use features from newer Python versions.

### Installing packages using RPM (Fedora / CentOS Stream)

When setting up CentOS Stream, please enable Code Ready Build and EPEL repositories:

```shell
sudo dnf install -y dnf-plugin-config-manager
sudo dnf config-manager -y --set-enabled crb
sudo dnf install -y epel-release
```

Then install the required packages:

```shell
dnf install \
    black \
    createrepo_c \
    podman \
    python3-isort \
    python3-flake8 \
    python3-paramiko \
    python3-podman \
    python3-pytest \
    python3-pyyaml \
    tmt \
    tmt+report-junit \
    -y
```

### Installing packages using pip (other operating systems)

Please install Python 3.9 environment and pip using standard methods on your operating system.
All additional required dependencies are listed in the [requirements.txt](./requirements.txt) and can be installed using `pip`:

```shell
pip install -U -r requirements.txt
```

Instead of installing the required packages directly, it is recommended to create a virtual environment. For example,
the following snippet uses the built-in [venv](https://docs.python.org/3/library/venv.html):

```shell
python -m venv ~/bluechi-env
source ~/bluechi-env/bin/activate
pip install -U -r requirements.txt
# ...

# exit the virtual env
deactivate
```

## Podman configuration

### Fix issue with pasta network provider

On Fedora 40 and newer podman 5 is installed and it uses new networking provider called pasta, but unfortunately it has some issue which prevents network connection between containers (more info in [Connectivity problem with Podman containers](https://discussion.fedoraproject.org/t/connectivity-problem-with-podman-containers/125664/32)).
To bypass this issue it's required to switch to previous networking provider called slirp4netns using following steps:

1. Install slirp4netns provider

   ```shell
   dnf install -y slirp4netns
   ```

2. Configure podman to use slirp4netns provider when executed under your username by creating `~/.config/containers/containers.conf` with following content:

   ```ini
   [Network]
   default_rootless_network_cmd = "slirp4netns"
   ```

### Configure podman socket access for users

Testing infrastructure uses socket access to podman, so it needs to be enabled:

```shell
systemctl --user enable podman.socket
systemctl --user start podman.socket
```

## Running integration tests

Integration tests are executed with [tmt framework](https://github.com/teemtee/tmt).

To run integration tests please execute below command in the [tests](./) directory:

```shell
tmt --feeling-safe run -v plan --name container
```

This will use latest BlueChi packages from
[bluechi-snapshot](https://copr.fedorainfracloud.org/coprs/g/centos-automotive-sig/bluechi-snapshot/) repository.

**Note:** The integration tests can be run in two modes - container and multi-host. For local execution it is advised to select `container` mode (hence the `plan --name container`).

## Running integration tests with memory leak detection

To run integration tests with `valgrind`, set `WITH_VALGRIND` environment variable as follows:

```shell
tmt --feeling-safe run -v -eWITH_VALGRIND=1 plan --name container
```

If `valgrind` detects a memory leak in a test, the test will fail, and the logs will be found in the test `data` directory.

## Running integration tests with local BlueChi build

In order to run integration tests for your local BlueChi build, you need have BlueChi RPM packages built from your source
code. The details about BlueChi development can be found at
[README.developer.md](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md), the most important part for
running integration tests is [Packaging](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md#packaging)
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
tmt --feeling-safe run -v -eCONTAINER_USED=integration-test-local plan --name container
```

## Creating code coverage report from integration tests execution

To be able to produce code coverage report from integration tests execution you need to build BlueChi RPMs with code
coverage support:

```shell
cd ~/bluechi
skipper make rpm WITH_COVERAGE=1
createrepo_c ~/bluechi/tests/bluechi-rpms
```

When done, you need to run integration tests with code coverage report enabled:

```shell
tmt --feeling-safe run -v -eCONTAINER_USED=integration-test-local -eWITH_COVERAGE=1 plan --name container
```

After the integration tests finishes, the code coverage html result can be found in `res` subdirectory inside the tmt
execution result directory, for example:

`/var/tmp/tmt/run-001/plans/tier0/report/default-0/report`

## Changing timeouts for integration tests

In some cases it might be necessary to adjust the default timeouts that are used in different steps of an integration tests execution cycle. The currently available environment variables as well as their default values are:

```shell
# in seconds
TIMEOUT_TEST_SETUP=20
TIMEOUT_TEST_RUN=45
TIMEOUT_COLLECT_TEST_RESULTS=20
```

These can be set either in the `environment` section of the tmt plan or using the `-e` option when running tmt, e.g. `-eTIMEOUT_TEST_SETUP=40`.

### Test-specific timeouts

In addition to the mentioned above timeouts, there's a mechanism allowing to set values in specific tests via environment variables, which can be used to adjust test-specific timeouts via the environment. The expected environment variable name would be assembled from the prefix `TEST_`, test name and a provided suffix. These test-specific timeouts can be set in the tmt plan as described [here](#changing-timeouts-for-integration-tests).

#### Example

Given a test named `bluechi-generic-test` (i.e. the test script located in the directory `bluechi-generic-test`), which uses a timeout `WAIT_TIMEOUT`, defining it as follows:

```python
WAIT_TIMEOUT = get_test_env_value_int("WAIT_TIMEOUT", 1000)
```

The variable would be assigned `WAIT_TIMEOUT = 1000`, unless environment variable `TEST_BLUECHI_GENERIC_TEST_WAIT_TIMEOUT` is set with an integer value, in which case this value would be passed to `WAIT_TIMEOUT`.

## Developing integration tests

### Code Style

Several tools are used in the project to validate code style:

- [flake8](https://pypi.org/project/flake8/) is used to enforce a unified code style.
- [isort](https://pypi.org/project/isort/) is used to enforce import ordering
- [black](https://pypi.org/project/black/) is used to enforce code formatting

All source files formatting can be checked/fixed using following commands executed from the top level directory of
the project:

```shell
flake8 tests
isort tests
black tests
```

### Changing log level

By default BlueChi integration tests are using `INFO` log level to display important information about the test run.
More detailed information can be displayed by setting log level to `DEBUG`:

```shell
cd ~/bluechi/tests
tmt --feeling-safe run -v -eLOG_LEVEL=DEBUG plan --name container
```

### Using python bindings in tests

The [python bindings](../src/bindings/python/) can be used in the integration tests to simplify writing them. However, it is not possible to use the bindings directly in the tests since they are leveraging the D-Bus API of BlueChi provided on the system D-Bus. A separate script has to be written, injected and executed within the container running the BlueChi controller. In order to keep the usage rather simple, the `BluechiControllerContainer` class provides a function to abstract these details:

```python
# run the file ./python/monitor.py located in the current test directory
# and get the exit code as well as the output (e.g. all print())
exit_code, output = ctrl.run_python("python/monitor.py")
```

A full example of how to use the python bindings can be found in the [monitor open-close test](./tests/tier0/monitor-open-close/).

### Generating test ID

Every test should be distinctly identified with a unique ID. Therefore, when adding a new test, please execute the following command to assign an ID to the new test:

```shell
$ cd ~/bluechi/tests
$ tmt test id .
New id 'UUID' added to test '/tests/path_to_your_new_test'.
...
```

### Checking for duplicate test IDs and summaries

In addition to having a unique ID, the summaries of tests should be descriptive and unique as well. The CI will perform appropriate linting. This can also be invoked locally:

```shell
$ cd ~/bluechi/tests

# requires tmt >= 1.35
$ tmt lint tests
Lint checks on all
fail G001 duplicate id "96aa0e17-5e23-4cc3-bc34-88368b8cc07b" in "/tests/tier0/bluechi-agent-connect-via-controller-address"
fail G001 duplicate id "96aa0e17-5e23-4cc3-bc34-88368b8cc07b" in "/tests/tier0/bluechi-agent-get-logtarget"
```

## Usage of containers

The integration tests rely on containers as separate compute entities. These containers are used to simulate BlueChi's
functional behavior on a single runner.

Both, [integration-test-local](./containers/integration-test-local) as well as [integration-test-snapshot](./containers/integration-test-snapshot), are based on the [integration-base](../containers/integration-test-base) image which contains core dependencies such as systemd and devel packages. The base image is published to [https://quay.io/repository/bluechi/integration-test-base](https://quay.io/repository/bluechi/integration-test-base).

### Updating container images in registry

The base images can either be build and pushed locally or via a github workflow to the [bluechi on quay.io](https://quay.io/organization/bluechi) organization and its repositories. If any updates are required, please reach out to the [code owners](../.github/CODEOWNERS).

#### Building and pushing via workflow

The base images [build-base](../containers/build-base) and [integration-test-base](../containers/integration-test-base) can be built and pushed to quay by using the [Container Image Workflow](../.github/workflows/images.yml). It can be found and triggered here in the [Actions tab](https://github.com/eclipse-bluechi/bluechi/actions/workflows/images.yml) of the BlueChi repo.

#### Building and pushing locally

The base images [build-base](../containers/build-base) and [integration-test-base](../containers/integration-test-base) are built for multiple architectures (arm64 and amd64) using the [build-containers.sh](../build-scripts/build-containers.sh) script. It'll build the images for the supported architectures as well as a manifest, which can then be pushed to the registry.

Building for multiple architectures, the following packages are required:

```shell
sudo dnf install -y podman buildah qemu-user-static
```

From the root directory of the project run the following commands:

```shell
# In order to build and directly push, login first
buildah login -u="someuser" -p="topsecret" quay.io
PUSH_MANIFEST=yes ./build-scripts/build-push-containers.sh build-base

# Only build locally
./build-scripts/build-push-containers.sh build-base
```

If you need to build only specific architecture for your local usage, you can specify it as the 2nd parameter:

```shell
./build-scripts/build-push-containers.sh build-base amd64
```
