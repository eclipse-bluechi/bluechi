<p align="center">
  <h2 align="center">Hirte</h3>
</p>

---

## Development

### Environment Setup

In order to develop the project you need to install following dependencies.

#### Prerequisities

To build the project on CentOS Stream 9 you need to enable CodeReady Build repository:
```bash
sudo dnf install -y dnf-plugin-config-manager
sudo dnf config-manager -y --set-enabled crb
```

#### Dependencies installation

```bash
sudo dnf install -y clang-tools-extra gcc make meson systemd-devel
```

### Code Style

[clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used to enforce a unified code style. All source files can be formatted via:
```bash
make fmt
```

A formatting check of existing files can be executed by:
```bash
make check-fmt
```

### Linting

[clang-tidy](https://clang.llvm.org/extra/clang-tidy/) is used for static analysis. All source files can be checked via:
```bash
make lint
```

Some errors detected by `clang-tidy` can be fixed automatically via: 
```bash
make lint-fix
```

### Build

The project is using [meson](https://mesonbuild.com/) build system.

The binaries can be built via
```bash
meson setup builddir
meson compile -C builddir
```

After successfully compiling the binaries, they can be installed into a destination directory (by default `/usr/local/bin`) using:
```bash
meson install
```

To install it into `builddir/bin` use:
```bash
meson install -C builddir --destdir bin
```

After building, three binaries are available:
- __hirte__: orchestrator, the central unit which is run on the main machine, sending commands to the nodes and monitoring the progress
- __hirte-node__: the executing unit which connects with the orchestrator and executes commands on the node machine
- __hirte-client__: a helper program to send an initial command to the orchestrator

### Unit tests

Unit tests can be executed using following commands
```bash
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

### Running

At the moment the hirte-client and hirte-node binary do only print a simple greeting and exit.

#### hirte

Hirte, the orchestrator can be run via:
```bash
./bin/hirte -P <port>
```
It starts a tcp socket and accepts connections, but does not do much more at this point.
This can be tested manually via
```bash
nc <host> <port>
```

#### hirte-node

Nodes can be run via:
```bash
./bin/hirte-node -H <host> -P <port>
```
It creates a new dbus which tries to connect to the specified host. The host will print out a message if the request was accepted. It does not do much more at this point.

## Documentation

For further documentation please refer to the [doc](./doc/) directory.

The target architecture for this project is described in [doc/arch](./doc/arch/)
