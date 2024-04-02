#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# orch build configuration

orch_src = [
    'controller.h',
    'controller.c',
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
  'bluechi-controller',
  orch_src,
  dependencies: [
    systemd_dep,
    inih_dep,
    hashmapc_dep,
  ],
  link_with: [
    bluechi_lib,
  ],
  install: true,
  install_dir: join_paths(prefixdir, get_option('libexecdir')),
  c_args: common_cflags,
  include_directories: include_directories('..')
)

# build test binaries
subdir('test')
