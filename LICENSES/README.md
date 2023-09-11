# BlueChi Project Licensing

## Main License

The BlueChi project uses single-line references to Unique License Identifiers as
defined by the Linux Foundation's [SPDX project](https://spdx.org/). The line in
each individual source file identifies the license applicable to that file.

The current set of valid, predefined SPDX identifiers can be found on the
[SPDX License List](https://spdx.org/licenses/).

The `LICENSES/` directory contains all the licenses used by the sources included in
the systemd project source tree.

Unless otherwise noted, the systemd project sources are licensed under the terms
and conditions of the **GNU LESSER GENERAL PUBLIC LICENSE Version 2.1 or later**.

New sources that cannot be distributed under **LGPL-2.1-or-later** will no longer
be accepted for inclusion in the BlueChi project to maintain license uniformity.

## Other Licenses

The following exceptions apply:

* Header file `src/libbluechi/common/list.h` is copied from [systemd](https://github.com/systemd/systemd) project
  and it's licensed under the original license, **LGPL-2.1-or-later**.
* Files `selinux/meson.build` and `selinux/build-selinux.sh` are copied from
  [flatpak](https://github.com/flatpak/flatpak) project and they are licensed under the original license,
  **LGPL-2.1-or-later**.
* Files under [`doc/api-examples`](../doc/api-examples/) are licensed under **MIT-0**
* Files under [`doc/bluechi-examples`](../doc/bluechi-examples/) are licensed under **MIT-0**
* Files under [`subprojects/`](../subprojects/) are integrated as git submodules and they are licensed under
their respective original license as follows:

  * [inih](https://github.com/benhoyt/inih/) located in [subprojects/inih](../subprojects/inih): **BSD-3-Clause**
  * [hashmap.c](https://github.com/tidwall/hashmap.c) located in [subprojects/hashmap.c](../subprojects/hashmap.c): **MIT**
