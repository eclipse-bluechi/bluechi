<!-- markdownlint-disable-file MD013 MD036 MD041 -->

**Setup sample project**

The C/C++ snippets using [libsystemd](https://www.freedesktop.org/software/systemd/man/libsystemd.html) require at least a simple project setup. Create it via:

```bash
# create project directory
mkdir bluechi
cd bluechi
```

**Installing dependencies**

The C/C++ snippets use the [libsystemd](https://www.freedesktop.org/software/systemd/man/libsystemd.html) library for the D-Bus bindings. For compilation `gcc` and/or `g++` can be used. Install it via:

```bash
# on Ubuntu
apt update
apt install libsystemd-dev build-essential
# on Fedora/CentOS
dnf install systemd-devel gcc g++
```

**Build and run the sample project**

The snippet chosen to run needs to be copied into the previously created the `bluechi` directory. After that the example can be build and run via:

```bash
# either build for C
gcc <filename>.c -g -Wall -o <filename> `pkg-config --cflags --libs libsystemd`
# or for C++
g++ <filename>.cpp -g -Wall -o <filename> `pkg-config --cflags --libs libsystemd`
# run the binary
./<filename>
```
