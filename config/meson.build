#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

install_data(
  install_dir : get_option('sysconfdir') / 'bluechi'
)

install_data(
  'agent/agent.conf',
  install_dir : join_paths(get_option('datadir'), 'bluechi-agent', 'config')
)

install_data(
  'controller/controller.conf',
  install_dir : join_paths(get_option('datadir'), 'bluechi', 'config')
)

install_data(
  'agent.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'bluechi' / 'agent.conf.d')
)

install_data(
  'controller.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'bluechi' / 'controller.conf.d')
)

install_data(
  'tmpfile/bluechi.conf',
  install_dir : join_paths(get_option('prefix'), 'lib', 'tmpfiles.d')
)
