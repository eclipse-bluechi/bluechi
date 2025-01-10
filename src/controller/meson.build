#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# controller build configuration

ctrl_src = [
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

controller_deps = [
    systemd_dep,
    inih_dep,
    hashmapc_dep,
  ]

if with_selinux
   controller_deps += [ selinux_dep, ]
endif

executable(
  'bluechi-controller',
  ctrl_src,
  dependencies: controller_deps,
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
