<!-- markdownlint-disable-file MD013 MD036 MD041 -->
**Installing Go**

First of all, install Go as described in the [official documentation](https://go.dev/doc/install).

**Setup sample project**

The Go snippets require at least a simple project setup. Create it via:

```bash
# create project directory
mkdir bluechi
cd bluechi

# initialize the project
go mod init bluechi
```

**Installing dependencies**

The Go snippets require [godbus](https://github.com/godbus/dbus/). Navigate into the `bluechi` directory and add it as a dependency via:

```bash
go get github.com/godbus/dbus/v5
```

**Build and run the sample project**

The snippet chosen to run needs to be copied into the `bluechi` directory. After that the example can be build and run via:

```bash
go build <filename>
./<filename> args...
```
