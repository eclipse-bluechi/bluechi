# SPDX-License-Identifier: GPL-2.0-or-later

# orch build configuration

orch_src = [
    'manager.h',
    'manager.c',
    'node.h',
    'node.c',
    'job.h',
    'job.c',
    'metrics.c',
    'metrics.h',
    'monitor.h',
    'monitor.c',
    'proxy_monitor.c',
    'proxy_monitor.h',
    'main.c',
]

executable(
  'hirte',
  orch_src,
  dependencies: [
    systemd_dep,
    inih_dep,
    hashmapc_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  install: true,
  c_args: common_cflags,
  include_directories: include_directories('..')
)
