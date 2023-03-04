# SPDX-License-Identifier: GPL-2.0-or-later

systemd_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'system')
systemd_user_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'user')

install_data(
  ['hirte.service', 'hirte-agent.service', 'hirte.socket', 'hirte-proxy@.service'],
  install_dir : systemd_unit_dir
)

install_data(
  ['hirte-agent-user.service', 'hirte-proxy-user@.service'],
  rename  : ['hirte-agent.service', 'hirte-proxy@.service'],
  install_dir : systemd_user_unit_dir
)
