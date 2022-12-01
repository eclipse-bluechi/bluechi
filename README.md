<p align="center">
  <h2 align="center">Project Name - TBD</h3>
</p>

---

## Usage

TBD

## Development

### Introduction

Project is using autotools, so build configuration needs to be generated first:

```bash
autoreconf -ivf
./configure
```

### Code Style

`clang-format` is used to enforce a unified code style. For setup instructions, please refer to the [official Clang getting started guide](https://clang.llvm.org/get_started.html).

Once `clang-format` is set up, all source files can be formatted via:
```bash
make fmt
```

### Build

The binaries can be built via
```bash
make all
```
and are output to `./bin/`.

After building, three binaries are available:
- __orch__: the central unit which is run on the main machine, sending commands to the nodes and monitoring the progress
- __node__: the executing unit which connects with the orchestrator and executes commands on the node machine
- __client__: a helper program to send an initial command to the orchestrator

### Running

At the moment the client and node binary do only print a simple greeting and exit.

#### orchestrator

The orchestrator can be run via:
```bash
./bin/orch <port>
```
It starts a tcp socket and accepts connections, but does not do much more at this point.
This can be tested manually via
```bash
nc <host> <port>
# e.g.
# nc localhost 1999
```

## Documentation

For further documentation please refer to the [doc](./doc/) directory.

The target architecture for this project is described in [doc/arch](./doc/arch/)
