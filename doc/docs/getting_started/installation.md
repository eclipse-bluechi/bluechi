<!-- markdownlint-disable-file MD010 MD013 MD014 MD024 MD046 -->
# Installation

The first step of getting started with BlueChi is the installation of it's components.

## Fedora or CentOS

On Fedora and CentOS-Stream systems (with [EPEL](https://docs.fedoraproject.org/en-US/epel/) repository enabled), all
components of BlueChi can be directly installed via:

```bash
dnf install bluechi-controller \
    bluechi-agent \
    bluechi-ctl \
    bluechi-selinux \
    python3-bluechi
```

## Debian-based distributions

On Debian systems, all components of BlueChi can be installed via:

```bash
# Add BlueChi PPA
curl -s --compressed "https://raw.githubusercontent.com/eclipse-bluechi/bluechi-ppa/main/deb/KEY.gpg" | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/bluechi.gpg >/dev/null
curl -s --compressed -o /etc/apt/sources.list.d/bluechi.list "https://raw.githubusercontent.com/eclipse-bluechi/bluechi-ppa/main/deb/bluechi.list"
sudo apt update

# Install BlueChi packages
sudo apt install bluechi-controller bluechi-agent bluechictl
```

The [typed python bindings](../api/client_generation.md#typed-python-client) are not provided as `.deb` package, but
can be installed from [pypi](https://pypi.org/project/bluechi/) via `pip`:

```bash
pip install bluechi
```

## Building from source

For building and packaging BlueChi from source, please refer to the respective section of the developer README on GitHub:

- [Building](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md#build)
- [Packaging](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md#packaging)
