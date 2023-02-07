# Hirte

## Development

### Environment Setup

In order to develop the project you need to install following dependencies.

#### Prerequisites

To build the project on CentOS Stream 9 you need to enable CodeReady Build repository:

```bash
sudo dnf install dnf-plugin-config-manager
sudo dnf config-manager --set-enabled crb
```

#### Dependencies installation

```bash
sudo dnf install clang-tools-extra gcc make meson systemd-devel
```

### Code Style

[clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used to enforce a unified code style. All source files can be formatted via:

```bash
make fmt
```

For the most part, this project follows systemd coding style: [systemd-coding-style][2].
Also, this project borrows some of the coding conventions from systemd.  For example, function names pertaining to D-Bus services look like `bus_service_set_property()`.

A formatting check of existing files can be executed by:

```bash
make check-fmt
```

### Linting

[clang-tidy][3] is used for static analysis. All source files can be checked via:

```bash
make lint
```

Some errors detected by `clang-tidy` can be fixed automatically via:

```bash
make lint-fix
```

### Build

The project is using [meson][1] build system.

The binaries can be built via

```bash
meson setup builddir
meson compile -C builddir
```

After successfully compiling the binaries, they can be installed into a destination directory (by default `/usr/local/bin`) using:

```bash
meson install -C builddir
```

To install it into `builddir/bin` use:

```bash
meson install -C builddir --destdir bin
```

After building, three binaries are available:

- __hirte__: the orchestrator which is run on the main machine, sending commands to the agents and monitoring the progress
- __hirte-agent__: the node agent unit which connects with the orchestrator and executes commands on the node machine
- __hirtectl__: a helper (CLI) program to send an commands to the orchestrator

### Unit tests

Unit tests can be executed using following commands

```bash
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

### Running

At the moment the hirtectl binary do only print a simple greeting and exit.

#### hirte

Hirte, the orchestrator can be run via:

```bash
hirte -c ./doc/example.ini
```

It starts a tcp socket and accepts connections,
but does not do much more at this point.
This can be tested manually via

```bash
nc <host> <port>
```

#### hirte-node

Nodes can be run via:

```bash
./bin/hirte-agent -n <agent name> -H <host> -P <port>
```

It creates a new dbus which tries to connect to the specified host.
The host will print out a message if the request was accepted.
It does not do much more at this point.

## Documentation

For further documentation please refer to the [doc](./doc/) directory.

The target architecture for this project is described in [doc/arch](./doc/arch/)

[1]: https://clang.llvm.org/docs/ClangFormat.html
[2]: https://github.com/systemd/systemd/blob/main/docs/CODING_STYLE.md
[3]: https://clang.llvm.org/extra/clang-tidy/
