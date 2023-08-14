# SPDX-License-Identifier: GPL-2.0-or-later

install_data(
  install_dir : get_option('sysconfdir') / 'bluechi'
)

install_data(
  'agent/agent.conf',
  install_dir : join_paths(get_option('datadir'), 'bluechi-agent', 'config')
)

install_data(
  'bluechi/bluechi.conf',
  install_dir : join_paths(get_option('datadir'), 'bluechi', 'config')
)

install_data(
  'agent.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'bluechi' / 'agent.conf.d')
)

install_data(
  'bluechi.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'bluechi' / 'bluechi.conf.d')
)
