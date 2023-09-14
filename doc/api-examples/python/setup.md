<!-- markdownlint-disable-file MD013 MD036 MD041 -->
**Installing Python**

First of all, install Python via:

```bash
# on Ubuntu
apt install python3 python3-dev
# on Fedora/CentOS
dnf install python3 python3-devel
```

**Installing dependencies**

The python snippets require the [dasbus](https://dasbus.readthedocs.io/en/latest/) package. Install it via pip:

```bash
# ensure pip has been installed
python3 -m ensurepip --default-pip
pip3 install dasbus
```

**Running the examples**

The snippets can be run by simply copy and pasting it into a new file running it via:

```bash
python3 <filename>.py args...
```
