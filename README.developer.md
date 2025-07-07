# Eclipse BlueChi&trade;

- [Development](#development)
  - [Environment Setup (Fedora / CentOS Stream)](#environment-setup)
    - [Prerequisites (CentOS Stream only)](#prerequisites)
    - [Dependencies Installation](#dependencies-installation)
  - [Busctl](#busctl)
  - [Code Style](#code-style)
  - [Linting](#linting)
  - [Build](#build)
    - [Build options](#build-options)
    - [Bindings](#bindings)
  - [Static Code Analysis](#static-code-analysis)
  - [Debug](#debug)
  - [Unit Tests](#unit-tests)
  - [Integration Tests](#integration-tests)
  - [Running](#running)
    - [Assumed setup](#assumed-setup)
    - [bluechi](#bluechi-controller)
    - [bluechi-agent](#bluechi-agent)
    - [bluechictl](#bluechictl)
- [Documentation](#documentation)
  - [Building MAN Pages](#building-man-pages)
- [Packaging](#packaging)
  - [RPM](#rpm)
  - [DEB](#deb)

## Development

### Environment Setup (Fedora / CentOS Stream) {#environment-setup}

In order to develop the project on Fedora or CentOS Stream you need to install following dependencies.

#### Prerequisites (CentOS Stream only) {#prerequisites}

To build the project on CentOS Stream you need to enable CodeReady Build and EPEL repositories:

```bash
sudo dnf install dnf-plugin-config-manager
sudo dnf config-manager --set-enabled crb
sudo dnf install -y epel-release
```

#### Dependencies installation

```bash
sudo dnf install \
    bzip2 \
    clang-tools-extra \
    gcc \
    gcc-c++ \
    git \
    golang-github-cpuguy83-md2man \
    make \
    meson \
    systemd-devel \
    selinux-policy-devel
```

[markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2) can be used for static analysis of markdown files.
Check the [installation guide](https://github.com/DavidAnson/markdownlint-cli2#install) and use the most appropriate way
of installation for your setup.

### busctl

**busctl** is a systemd tool to introspect and monitor the D-Bus bus. See below some examples using busctl with bluechi service.

**Instrospect org.eclipse.bluechi**:

```bash
# busctl introspect \
         org.eclipse.bluechi \
         /org/eclipse/bluechi

NAME                                TYPE      SIGNATURE RESULT/VALUE   FLAGS
org.eclipse.bluechi.Controller         interface -         -              -
.CreateMonitor                      method    -         o              -
.DisableMetrics                     method    -         -              -
.EnableMetrics                      method    -         -              -
.GetNode                            method    s         o              -
.ListNodes                          method    -         a(sos)         -
.ListUnits                          method    -         a(sssssssouso) -
.Ping                               method    s         s              -
.SetLogLevel                        method    s         -              -
.JobNew                             signal    uo        -              -
.JobRemoved                         signal    uosss     -              -
org.freedesktop.DBus.Introspectable interface -         -              -
.Introspect                         method    -         s              -
org.freedesktop.DBus.Peer           interface -         -              -
.GetMachineId                       method    -         s              -
.Ping                               method    -         -              -
org.freedesktop.DBus.Properties     interface -         -              -
.Get                                method    ss        v              -
.GetAll                             method    s         a{sv}          -
.Set                                method    ssv       -              -
.PropertiesChanged                  signal    sa{sv}as  -              -
```

**Example calling ListNodes**:

```bash
export SERVICE="org.eclipse.bluechi"
export OBJECT="/org/eclipse/bluechi"
export INTERFACE="org.eclipse.bluechi.Controller"
export METHOD="ListNodes"

# busctl call \
	"${SERVICE}" \
	"${OBJECT}" \
	"${INTERFACE}" \
	"${METHOD}"

a(sos) 3 "control" "/org/eclipse/bluechi/node/control" "online" "node1" "/org/eclipse/bluechi/node/node1" "online" "qm-node1" "/org/eclipse/bluechi/node/qm_2dnode1" "online"
```

### Code Style

[clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used to enforce a uniform code style. The formatting
of all source files can be checked via:

```bash
# only check for formatting
make check-fmt

# apply formatting to files
make fmt
```

For the most part, this project follows systemd coding style:
[systemd-coding-style](https://github.com/systemd/systemd/blob/main/docs/CODING_STYLE.md). Also, this project borrows
some of the coding conventions from systemd. For example, function names pertaining to D-Bus services look like
`bus_service_set_property()`.

### Linting

[clang-tidy](https://clang.llvm.org/extra/clang-tidy/) is used for static analysis of C source codes.
[markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2) is used for static analysis of markdown files.
[flake8](https://flake8.pycqa.org/en/latest/) is used for format checking and linting of python code.

The following make targets simplify the usage of the different linting tools:

```bash
# using clang-tidy, lint all C source files
make lint-c

# same as lint-c target, but auto-fix errors if possible
make lint-c-fix

# using markdownlint-cli2, lint all markdown files
make lint-markdown

# combines lint-c and lint-markdown
make lint
```

### Build

The project is using [meson](https://mesonbuild.com/) as its primary build system.

The binaries and other artifacts (such as the man pages) can be built via:

```bash
meson setup builddir
meson compile -C builddir
```

After successfully compiling the binaries, they can be installed into a destination directory (by default
`/usr/local/bin`) using:

```bash
meson install -C builddir
```

To install it into `builddir/bin` use:

```bash
meson install -C builddir --destdir bin
```

After building, the following binaries are available:

- `bluechi-controller`: the systemd service controller which is run on the main machine, sending commands to the agents
  and monitoring the progress
- `bluechi-agent`: the node agent unit which connects with the controller and executes commands on the node machine
- `bluechi-proxy`: an internally used application to resolve cross-node dependencies
- `bluechictl`: a helper (CLI) program to send an commands to the controller

#### Build options

BlueChi can be built with configurable options as listed in [meson_options.txt](./meson_options.txt). The value for
those settings can either be changed directly in the file or via

```bash
# assuming an initial "meson setup builddir"
meson configure -D<option-name>=<option-value> builddir
```

Current options include:

- `with_analyzer`: This option enables the [gcc option for static analysis](https://gcc.gnu.org/onlinedocs/gcc-13.2.0/gcc/Static-Analyzer-Options.html)
- `with_coverage`: This option ensures that BlueChi is built to collect coverage when running a BlueChi binary
- `with_man_pages`: This option enables building man pages as a part of the project build
- `with_selinux`: This option includes building the SELinux policy for BlueChi

#### Bindings

Bindings for the D-Bus API of `BlueChi` are located in [src/bindings](./src/bindings/). Please refer to the
[README.md](./src/bindings/README.md) for more details.

A complete set of typed python bindings for the D-Bus API is auto-generated. On any change to any of the [interfaces](./data/),
these need to be re-generated via

```bash
./build-scripts/generate-bindings.sh python
```

### Static Code Analysis

Additional, static code analysis is applied when running the [unit tests in CI](./.github/workflows/unit-tests.yml) by
enabling the meson option `with_analyzer`:

```bash
meson configure -Dwith_analyzer=true builddir
```

Since the result of the analysis is heavily dependent on the compiler version, flags, etc., its recommended to build
and run tests for BlueChi also in the [build-base](./containers/build-base):

```bash
# navigate into the bluechi directory
cd bluechi

# run build-base container and mount bluechi directory
podman run -it -v ./:/var/bluechi quay.io/bluechi/build-base:stream10

# configure meson to enable code analysis and build the project
meson setup builddir
meson configure -Dwith_analyzer=true builddir
meson compile -C builddir
```

### Debug

In some cases, developers might need a debug session with tools like gdb, here an example:

First, make sure **meson.build** contains **debug=true**.

Rebuild the BlueChi project with debug symbols included:

```bash
bluechi> make clean
bluechi> meson install -C builddir --dest=bin
bluechi> gdb --args ./builddir/bin/usr/local/libexec/bluechi-controller -c /etc/bluechi/controller.conf
```

### Unit tests

Unit tests can be executed using following commands:

```bash
meson setup builddir
meson configure -Db_coverage=true builddir  # if you want to collect coverage data
meson compile -C builddir
meson test -C builddir
ninja coverage-html -C builddir  # if you want to generate coverage report
```

will produce a coverage report in `builddir/meson-logs/coveragereport/index.html`

### Integration tests

All files related to the integration tests are located in [tests](./tests/) and are organized via
[tmt](https://tmt.readthedocs.io/en/stable/). How to get started with running and developing
integration tests for BlueChi is described in [tests/README](./tests/README.md).

### Running

The following sections describe how to run the built application(s) locally on one machine. For this, the assumed setup
used is described in the first section.

#### Assumed setup

The project has been build with the following command sequence:

```bash
meson setup builddir
meson compile -C builddir
meson install -C builddir --destdir bin
```

Meson will output the artifacts to `./builddir/bin/usr/local/`. This directory is referred to in the following sections
simply as `<builddir>`.

To allow `bluechi-controller` and `bluechi-agent` to own a name on the local system D-Bus, the provided configuration
files need to be copied (if not already existing):

```bash
cp <builddir>/share/dbus-1/system.d/org.eclipse.bluechi.Agent.conf /etc/dbus-1/system.d/
cp <builddir>/share/dbus-1/system.d/org.eclipse.bluechi.conf /etc/dbus-1/system.d/
```

**Note:** Make sure to reload the dbus service so these changes take effect: `systemctl reload dbus-broker.service` (or
`systemctl reload dbus.service`)

#### bluechi-controller

The newly built controller can simply be run via `./<builddir>/bin/bluechi-controller`, but it is recommended to use a
specific configuration for development. This file can be passed in with the `-c` CLI option:

```bash
./<builddir>/bin/bluechi-controller -c <path-to-cfg-file>
```

#### bluechi-agent

Before starting the agent, it is best to have the `bluechi-controller` already running. However, `bluechi-agent` will
try to reconnect in the configured heartbeat interval.

Similar to `bluechi-controller`, it is recommended to use a dedicated configuration file for development:

```bash
./<builddir>/bin/bluechi-agent -c <path-to-cfg-file>
```

#### bluechictl

The newly built `bluechictl` can be used via:

```bash
./<builddir>/bin/bluechictl COMMANDS
```

## Documentation

Files for documentation of this project are located in the [doc](./doc/) directory comprising:

- [api examples](./doc/api-examples/): directory containing source files for different programming languages that use
  the D-Bus API of BlueChi, e.g. for starting a systemd unit
- [man](./doc/man/): directory containing the markdown files for generating the man pages
  (see [Building MAN pages](#building-man-pages) for more information)
- readthedocs files for building the documentation website of BlueChi (see [the README](./doc/README.md) for further information)
- [diagrams.drawio](./doc/diagrams.drawio) file containing all diagrams used for BlueChi

### Building MAN pages

The source markdown files for building the MAN pages are located in [doc/man](./doc/man/). For generating the MAN pages
from the markdown files [go-md2man](https://github.com/cpuguy83/go-md2man) is being used. Install it via:

```bash
sudo dnf install golang-github-cpuguy83-md2man
```

After executing a `meson install` the MAN pages are located in the `man/man*` directories.

## Packaging

### RPM

In order to package BlueChi as an RPM the following dependencies are required:

```bash
sudo dnf install \
    jq \
    rpm-build
```

The scripts used to create the SRPM and RPM may be found in [build-scripts](./build-scripts) directory. Executing
following command will create RPM packages for the current content of the project:

```bash
sudo ./build-scripts/build-rpm.sh
```

Created RPM packages can be found under `artifacts/rpms*` subdirectories according to the timestamp of the execution.

The package build process can be adjusted using following environment variables:

- `ARTIFACTS_DIR`
  - A full path to a directory where the RPMs will be saved. The script will create the directory if it does not exist.
- `SKIP_BUILDDEP`
  - To skip installation of build dependencies this option should contain `yes` value.
- `WITH_PYTHON`
  - To skip building python bluechi modules this option should contain `0`. Default: `1`.
- `WITH_COVERAGE`
  - To start collecting coverage this option should contain `1`. Default: `0`.

So for example following command will skip build dependencies installation and store create RPM packages into `output`
subdirectory:

```bash
SKIP_BUILDDEP=yes ARTIFACTS_DIR=${PWD}/output ./build-scripts/build-rpm.sh
```

### DEB

Creating the BlueChi `.deb` package is described in the [debian/README.md](./debian/README.md).
