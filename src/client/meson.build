# SPDX-License-Identifier: GPL-2.0-or-later

# client build configuration

client_src = [
  'main.c',
  'client.c',
  'printer.c',
  'method_monitor.c'
]

executable(
  'hirtectl',
  client_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  c_args: common_cflags,
  install: true,
  include_directories: include_directories('..')
)
