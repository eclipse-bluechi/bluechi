#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# proxy build configuration

proxy_src = [
  'main.c',
]

executable(
  'bluechi-proxy',
  proxy_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    bluechi_lib,
  ],
  c_args: common_cflags,
  include_directories: include_directories('..'),
  install: true,
  install_dir: join_paths(prefixdir, get_option('libexecdir'))
)
