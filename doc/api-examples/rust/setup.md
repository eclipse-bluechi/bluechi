<!-- markdownlint-disable-file MD013 MD036 MD041 -->
**Installing Rust**

First of all, install Rust as describe in the [official documentation](https://www.rust-lang.org/tools/install).

**Installing dependencies**

The Rust snippets requires [dbus](https://docs.rs/dbus/latest/dbus/index.html).
Since this crate needs the reference implementation `libdbus`, it has to be installed:

```bash
# on Ubuntu
apt install libdbus-1-dev pkg-config
# on Fedora/CentOS
dnf install dbus-devel pkgconf-pkg-config
```

**Build and run the sample project**

The snippet chosen to run needs to be copied into the `bluechi` directory. After that the example can be build and run via:

```bash
# clone the BlueChi project
$ git clone https://github.com/eclipse-bluechi/bluechi.git

# navigate into the rust api-examples directory
$ cd bluechi/doc/api-examples/rust

# build all example applications
$ cargo build

# run a specific example, e.g. start-unit
$ ./target/debug/start-unit laptop cow.service
$ ...
```
