# SPDX-License-Identifier: GPL-2.0-or-later

install_data(
  install_dir : get_option('sysconfdir') / 'hirte'
)

install_data(
  'agent/agent.conf',
  install_dir : join_paths(get_option('datadir'), 'hirte-agent', 'config')
)

install_data(
  'hirte/hirte.conf',
  install_dir : join_paths(get_option('datadir'), 'hirte', 'config')
)

install_data(
  'agent.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'hirte' / 'agent.conf.d')
)

install_data(
  'hirte.conf.d/README.md',
  install_dir :  join_paths(get_option('sysconfdir') / 'hirte' / 'hirte.conf.d')
)
