#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

is_online_src = [
  'main.c',
  'help.c',
  'is-online.c',
]

executable(
  'bluechi-is-online',
  is_online_src,
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

# build test binaries
# subdir('test') 
