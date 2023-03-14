# SPDX-License-Identifier: GPL-2.0-or-later

systemd_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'system')

install_data(
  'hirte.service',
  install_dir : systemd_unit_dir
)

install_data(
  'hirte.socket',
  install_dir : systemd_unit_dir
)

install_data(
  'hirte-agent.service',
  install_dir : systemd_unit_dir
)
