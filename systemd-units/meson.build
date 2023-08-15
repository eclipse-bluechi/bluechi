# SPDX-License-Identifier: LGPL-2.1-or-later

systemd_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'system')
systemd_user_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'user')

install_data(
  ['bluechi.service', 'bluechi-agent.service', 'bluechi.socket', 'bluechi-proxy@.service', 'bluechi-dep@.service'],
  install_dir : systemd_unit_dir
)

install_data(
  ['bluechi-agent-user.service', 'bluechi-proxy-user@.service', 'bluechi-dep@.service'],
  rename  : ['bluechi-agent.service', 'bluechi-proxy@.service', 'bluechi-dep@.service'],
  install_dir : systemd_user_unit_dir
)
