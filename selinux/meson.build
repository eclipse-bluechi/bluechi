# Copyright 2022 Collabora Ltd.
# SPDX-License-Identifier: LGPL-2.1-or-later

custom_target(
  'hirte.pp.bz2',
  input : ['hirte.te', 'hirte.fc'],
  output : ['hirte.pp.bz2'],
  build_by_default : true,
  command : [
    files('build-selinux.sh'),
    '@OUTPUT0@',
    '@INPUT@',
  ],
  install : true,
  install_dir : join_paths(get_option('datadir'), 'selinux', 'packages'),
)

install_data(
  'hirte.if',
  install_dir : join_paths(get_option('datadir'), 'selinux', 'devel', 'include', 'services'),
)