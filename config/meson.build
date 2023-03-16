# SPDX-License-Identifier: GPL-2.0-or-later

install_data(
  ['hirte/hirte.conf', 'agent/agent.conf'],
  install_dir : get_option('sysconfdir') / 'hirte'
)

install_data(
  'agent/hirte-default.conf',
  install_dir : join_paths(get_option('datadir'), 'hirte-agent', 'config')
)

install_data(
  'hirte/hirte-default.conf',
  install_dir : join_paths(get_option('datadir'), 'hirte', 'config')
)
