# SPDX-License-Identifier: LGPL-2.1-or-later

# client build configuration

client_src = [
  'main.c',
  'client.c',
  'method_monitor.c',
  'method_list_units.c',
  'method_status.c'
]

executable(
  'bluechictl',
  client_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    bluechi_lib,
  ],
  c_args: common_cflags,
  install: true,
  include_directories: include_directories('..')
)
