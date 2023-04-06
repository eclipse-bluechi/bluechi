# Hirte Project Licensing

## Main License

The Hirte project uses single-line references to Unique License Identifiers as
defined by the Linux Foundation's [SPDX project](https://spdx.org/). The line in
each individual source file identifies the license applicable to that file.

The current set of valid, predefined SPDX identifiers can be found on the
[SPDX License List](https://spdx.org/licenses/).

The `LICENSES/` directory contains all the licenses used by the sources included in
the systemd project source tree.

Unless otherwise noted, the systemd project sources are licensed under the terms
and conditions of the **GNU GENERAL PUBLIC LICENSE Version 2**.

New sources that cannot be distributed under **GPL-2.0-or-later** will no longer
be accepted for inclusion in the Hirte project to maintain license uniformity.

## Other Licenses

The following exceptions apply:

* Header file `src/libhirte/common/list.h` is copied from [systemd](https://github.com/systemd/systemd) project
  and it's licensed under the original license, **LGPL-2.1-or-later**.
* Files `selinux/meson.build` and `selinux/build-selinux.sh` are copied from
  [flatpak](https://github.com/flatpak/flatpak) project and they are licensed under the original license,
  **LGPL-2.1-or-later**.
* Files under `src/libhirte/ini/` are copied from [inih](https://github.com/benhoyt/inih) project and they're
  licensed under the original license, **BSD-3-Clause**.
* Files under `src/libhirte/hashmap/` are copied from [hashmap.c](https://github.com/tidwall/hashmap.c) project
  and they are licensed under the original license, **MIT**.
