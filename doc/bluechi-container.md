<!-- markdownlint-disable-file MD013-->
# Table of Contents

1. [Cloning Bluechi in your Laptop to Start the Development Container](#cloning-bluechi-in-your-laptop-to-start-the-development-container)
2. [Inside the Container](#inside-the-container)
    - [Building Bluechi with Meson Inside Container](#building-bluechi-with-meson-inside-container)
    - [Locating the src.rpm and .rpm Generated](#locating-the-srcrpm-and-rpm-generated)
3. [Building All Unit Tests](#building-all-unit-tests)
    - [Listing all unit tests](#listing-all-unit-tests)
    - [Building single unit test](#building-single-unit-test)
4. [Building All Integration Tests](#building-all-integration-tests)
    - [Assumed Setup](#assumed-setup)
    - [Running the Controller and Agent](#running-the-controller-and-agent)
5. [Building a Unique TMT Test (Integration Test in Python)](#building-a-unique-tmt-test-integration-test-in-python)

---

# Cloning Bluechi in your Laptop to Start the Development Container

First, we need to clone Bluechi repo to use the ContainerFile (build-base below) that Bluechi developers use for development and CI/CD.

```bash
git clone git@github.com:eclipse-bluechi/bluechi.git
cd bluechi/
podman build -t build-base . -f ./containers/build-base
podman run -it --rm build-base
```

- The `-t` flag names the image `build-base`.
- The `-it` flag runs the container interactively.
- The `--rm` flag ensures the container is removed after you exit it.

## Inside the Container

Now that the container image is built and the container is running, let's execute the next steps in this environment, which acts as a "virtualization" environment.

### Building Bluechi with Meson Inside Container

Now we need to clone the project again, but inside the container, and build it using Meson.

```bash
[root@8b9162d77132 /]# cd /root
[root@8b9162d77132 ~]# git clone https://github.com/eclipse-bluechi/bluechi.git
Cloning into 'bluechi'...
remote: Enumerating objects: 11425, done.
remote: Counting objects: 100% (1996/1996), done.
remote: Compressing objects: 100% (669/669), done.
remote: Total 11425 (delta 1387), reused 1426 (delta 1325), pack-reused 9429 (from 1)
Receiving objects: 100% (11425/11425), 5.58 MiB | 16.00 MiB/s, done.
Resolving deltas: 100% (7552/7552), done.
[root@8b9162d77132 ~]# cd bluechi/
[root@8b9162d77132 bluechi]# meson setup builddir
[root@8b9162d77132 bluechi]# meson compile -C builddir
```

### Locating the src.rpm and .rpm Generated

```bash
[root@8b9162d77132 bluechi]# find . -name *.rpm
./artifacts/rpms-202411191517/bluechi-0.10.0-0.202411191516.gitd5af188.el9.src.rpm
./artifacts/rpms-202411191517/bluechi-controller-debuginfo-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-ctl-debuginfo-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-debuginfo-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-controller-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-debugsource-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-is-online-debuginfo-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-ctl-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-agent-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-is-online-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/bluechi-agent-debuginfo-0.10.0-0.202411191516.gitd5af188.el9.x86_64.rpm
./artifacts/rpms-202411191517/python3-bluechi-0.10.0-0.202411191516.gitd5af188.el9.noarch.rpm
./artifacts/rpms-202411191517/bluechi-selinux-0.10.0-0.202411191516.gitd5af188.el9.noarch.rpm
```

## Building All Unit Tests

```bash
meson setup builddir
meson configure -Db_coverage=true builddir  # if you want to collect coverage data
meson compile -C builddir
meson test -C builddir
ninja coverage-html -C builddir  # if you want to generate coverage report
```

Source: [Bluechi Developer README](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md#unit-tests)

## Listing all unit tests

```bash
meson test -C builddir --list
bus_id_is_valid_test
command_add_option_test
command_flag_exists_test
command_get_option_test
command_get_option_long_test
get_option_type_test
command_execute_test
methods_get_method_test
list_test
parse-util_test
time-util_test
cfg_def_conf_test
cfg_get_set_test
cfg_load_complete_configuration_test
cfg_load_from_env_test
cfg_load_from_file_test
is_ipv4_test
is_ipv6_test
get_address_test
assemble_tcp_address
match_glob_test
ends_with_test
get_log_timestamp
micros_to_millis_test
timespec_to_micros_test
bc_log_init_test
bc_log_to_stderr_full_test
bc_log_to_stderr_test
log_target_to_str_test
string_to_log_level_test
socket_option_test
socket_set_options_test
controller_apply_config_test
agent_apply_config_test
```

## Building single unit test

```bash
meson test -C builddir agent_apply_config_test
ninja: Entering directory `/home/douglas/jira/bluechi/builddir'
ninja: no work to do.
1/1 agent_apply_config_test        OK              0.01s

Ok:                 1
Expected Fail:      0
Fail:               0
Unexpected Pass:    0
Skipped:            0
Timeout:            0
```

## Building All Integration Tests

All files related to the integration tests are located in `tests` and are organized via `tmt`. How to get started with running and developing integration tests for BlueChi is described in `tests/README`.

### Assumed Setup

The project has been built with the following command sequence:

```bash
meson setup builddir
meson compile -C builddir
meson install -C builddir --destdir bin
```

Meson will output the artifacts to `./builddir/bin/usr/local/`. This directory is referred to in the following sections simply as `<builddir>`.

To allow `bluechi-controller` and `bluechi-agent` to own a name on the local system D-Bus, the provided configuration files need to be copied (if not already existing):

```bash
cp <builddir>/share/dbus-1/system.d/org.eclipse.bluechi.Agent.conf /etc/dbus-1/system.d/
cp <builddir>/share/dbus-1/system.d/org.eclipse.bluechi.conf /etc/dbus-1/system.d/
```

Note: Make sure to reload the D-Bus service so these changes take effect:

```bash
systemctl reload dbus-broker.service
# or
systemctl reload dbus.service
```

### Running the Controller and Agent

- The newly built controller can simply be run via:

  ```bash
  ./<builddir>/bin/bluechi-controller
  ```

  It is recommended to use a specific configuration for development. This file can be passed in with the `-c` CLI option:

  ```bash
  ./<builddir>/bin/bluechi-controller -c <path-to-cfg-file>
  ```

- Before starting the `bluechi-agent`, it is best to have the `bluechi-controller` already running. However, `bluechi-agent` will try to reconnect in the configured heartbeat interval.

  Similar to `bluechi-controller`, it is recommended to use a dedicated configuration file for development:

  ```bash
  ./<builddir>/bin/bluechi-agent -c <path-to-cfg-file>
  ```

- The newly built `bluechictl` can be used via:

  ```bash
  ./<builddir>/bin/bluechictl COMMANDS
  ```

Source: [Bluechi Developer README](https://github.com/eclipse-bluechi/bluechi/blob/main/README.developer.md#integration-tests)

## Building a Unique TMT Test (Integration Test in Python)

```bash
tmt --feeling-safe run -v plan --name container
```

The `plan --name container` tells the test suite to use the container variant (the other variant would be multi-host, but that requires multiple PCs). The `--feeling-safe` option is required for the newest `tmt` version—they want to prevent running locally provisioned tests by accident. But BlueChi's integration tests with containers need it.

You can run one specific test by adding `test --name <regex-of-test-directory>`, e.g.:

```bash
tmt --feeling-safe run -v plan --name container test --name bluechi-agent-cmd-options
```
