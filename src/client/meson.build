#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# client build configuration

client_src = [
  'main.c',
  'client.c',
  'method-help.c',
  'method-is-enabled.c',
  'method-loglevel.c',
  'method-list-unit-files.c',
  'method-list-units.c',
  'method-metrics.c',
  'method-monitor.c',
  'method-unit-lifecycle.c',
  'method-status.c',
  'method-kill.c',
  'method-freeze-thaw.c',
  'method-enable-disable.c',
  'method-reset-failed.c',
  'method-daemon-reload.c',
  'method-default-target.c',
  'usage.c',
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
