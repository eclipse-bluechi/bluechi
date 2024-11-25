<!-- markdownlint-disable-file MD013 MD036 MD041 -->

**Setup sample project**

The C++ snippets with [sdbus-c++](https://github.com/Kistler-Group/sdbus-cpp/) require at least a simple project setup. Create it via:

```bash
# create project directory
mkdir bluechi
cd bluechi
```

**Installing dependencies**

The C++ snippets use the [sdbus-c++](https://github.com/Kistler-Group/sdbus-cpp/) library **v2.0** for the D-Bus bindings. For compilation `g++` can be used. Install it via:

```bash
# on Ubuntu
apt update
apt install libsystemd-dev build-essential libsdbus-c++-dev
# on Fedora/CentOS
dnf install systemd-devel g++ sdbus-cpp
```

Or build and install it according to the instructions in the [sdbus-cpp repository](https://github.com/Kistler-Group/sdbus-cpp?tab=readme-ov-file#building-and-installing-the-library).

**Build and run the sample project**

The snippet chosen to run needs to be copied into the previously created the `bluechi` directory. After that the example can be build and run via:

```bash
# build the binary
g++ <filename>.cpp -g -Wall -o <filename> `pkg-config --cflags --libs sdbus-c++`
# run the binary
./<filename>
```
