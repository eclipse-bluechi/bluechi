# SPDX-License-Identifier: LGPL-2.1-or-later

# node build configuration

node_src = [
  'main.c',
  'agent.c',
  'proxy.c'
]

executable(
  'bluechi-agent',
  node_src,
  dependencies: [
    systemd_dep,
    inih_dep,
    hashmapc_dep,
  ],
  link_with: [
    bluechi_lib,
  ],
  c_args: common_cflags,
  include_directories: include_directories('..'),
  install: true,
  install_dir: join_paths(prefixdir, get_option('libexecdir'))
)
