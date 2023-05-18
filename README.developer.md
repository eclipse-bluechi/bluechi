# Hirte

## Development

### Environment Setup

In order to develop the project you need to install following dependencies.

#### Prerequisites

To build the project on CentOS Stream 9 you need to enable CodeReady Build repository:

```bash
sudo dnf install dnf-plugin-config-manager
sudo dnf config-manager --set-enabled crb
sudo dnf install -y epel-release
```

#### Dependencies installation

```bash
sudo dnf install clang-tools-extra gcc make meson systemd-devel selinux-policy-devel
```

[markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2) can be used for static analysis of markdown files.
Check the [installation guide](https://github.com/DavidAnson/markdownlint-cli2#install) and use the most appropriate way
of installation for your setup.

### Code Style

[clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used to enforce a unified code style. All source files
can be formatted via:

```bash
make fmt
```

For the most part, this project follows systemd coding style:
[systemd-coding-style](https://github.com/systemd/systemd/blob/main/docs/CODING_STYLE.md). Also, this project borrows
some of the coding conventions from systemd. For example, function names pertaining to D-Bus services look like
`bus_service_set_property()`.

A formatting check of existing files can be executed by:

```bash
make check-fmt
```

### Linting

[clang-tidy](https://clang.llvm.org/extra/clang-tidy/) is used for static analysis of C source codes.
[markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2) is used for static analysis of markdown files.

All source files can be checked via:

```bash
make lint
```

Some errors detected by `clang-tidy` can be fixed automatically via:

```bash
make lint-fix
```

### Build

The project is using [meson](https://mesonbuild.com/) build system.

The binaries can be built via:

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

After building, three binaries are available:

- `hirte`: the systemd service controller which is run on the main machine, sending commands to the agents and
  monitoring the progress
- `hirte-agent`: the node agent unit which connects with the controller and executes commands on the node machine
- `hirtectl`: a helper (CLI) program to send an commands to the controller

### Unit tests

Unit tests can be executed using following commands

```bash
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

### Integration tests

All files related to the integration tests are located in [tests](./tests/) and are organized via
[tmt](https://tmt.readthedocs.io/en/stable/). How to get started with running and developing
integration tests for hirte is described in [tests/README](./tests/README.md).

### Running

At the moment the `hirtectl` binary does not implement any logic. It only prints a simple greeting and exit.

#### hirte

The controller can be run via:

```bash
hirte
```

It starts a tcp socket on all network interfaces on port `842` and accepts connections, but does not do much more at
this point. This can be tested manually via:

```bash
nc <host> <port>
```

#### hirte-agent

A node can be run via:

```bash
./bin/hirte-agent -n <agent name> -H <host>
```

It creates a new D-Bus which tries to connect to the specified host. The host will print out a message if the request
was accepted. It does not do much more at this point.

## Documentation

For further documentation please refer to the [doc](./doc/) directory.

The target architecture for this project is described in [doc/arch](./doc/arch/).

### Building MAN pages

The source markdown files for building the MAN pages are located in [doc/man](./doc/man/). For generating the MAN pages
from the markdown files [go-md2man](https://github.com/cpuguy83/go-md2man) is being used. Install it via:

```bash
sudo dnf install golang-github-cpuguy83-md2man
```

After executing a `meson install` the MAN pages are located in the `man/man*` directories.

## Packaging

Hirte is packaged as an RPM. The scripts used to create the SRPM and RPM may be found in [.build-scripts](./build-scripts).

The RPM creation [script](./build-scripts/build-rpm.sh) requires one environment variable:

- `ARTIFACTS_DIR` - full path to a directory where the RPMs will be saved.
  The script will create the directory if it does not exist

Please note that the RPM creation script also installs all build dependencies and therefore requires running with root privileges

In order to build an RPM locally use the following command:

```bash
sudo ARTIFACTS_DIR=${PWD}/output ./build-scripts/build-rpm.sh
```

Alternatively, by setting the environment variable `SKIP_BUILDDEP=yes`,
the installation process is skipped, allowing regular user permissions

```bash
SKIP_BUILDDEP=yes ARTIFACTS_DIR=${PWD}/output ./build-scripts/build-rpm.sh
```
