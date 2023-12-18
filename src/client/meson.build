# SPDX-License-Identifier: LGPL-2.1-or-later

# client build configuration

client_src = [
  'main.c',
  'client.c',
  'method-help.c',
  'method-loglevel.c',
  'method-list-units.c',
  'method-metrics.c',
  'method-monitor.c',
  'method-unit-actions.c',
  'method-status.c'
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
