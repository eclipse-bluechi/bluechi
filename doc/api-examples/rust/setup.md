<!-- markdownlint-disable-file MD013 MD036 MD041 -->
**Installing Rust**

First of all, install Rust as describe in the [official documentation](https://www.rust-lang.org/tools/install).

**Setup sample project**

The Rust snippets require at least a simple project setup. Create it via:

```bash
# initialize the project
cargo new bluechi
cd bluechi
```

**Installing dependencies**

The Rust snippets require the [dbus](https://docs.rs/dbus/latest/dbus/index.html) create. Navigate into the `bluechi` directory and add it as a dependency via:

```bash
cargo add dbus
```

Since the [dbus](https://docs.rs/dbus/latest/dbus/index.html) create requires the reference implementation `libdbus`, this needs to be installed as well:

```bash
# on Ubuntu
apt install libdbus-1-dev pkg-config
# on Fedora/CentOS
dnf install dbus-devel pkgconf-pkg-config
```

**Build and run the sample project**

The snippet chosen to run needs to be copied into the `bluechi` directory. After that the example can be build and run via:

```bash
cargo build
./target/debug/bluechi args...
```
