#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

systemd_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'system')
systemd_user_unit_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'user')

install_data(
  [
    'bluechi-controller.service',
    'bluechi-agent.service',
    'bluechi-proxy@.service',
    'bluechi-dep@.service'
    ],
  install_dir : systemd_unit_dir
)

conf = configuration_data()
conf.set('unix_domain_socket_path', get_option('unix_domain_socket_path'))

configure_file(
    output: 'bluechi-controller.socket',
    input: 'bluechi-controller.socket.in',
    configuration: conf,
    install_dir: systemd_unit_dir,
)

# Meson does not have a variable for the /run directory.
#   see: https://mesonbuild.com/Builtin-options.html#directories
install_subdir(
    'bluechi',
    install_dir: '/run/',
)

install_data(
  ['bluechi-agent-user.service', 'bluechi-proxy-user@.service', 'bluechi-dep@.service'],
  rename  : ['bluechi-agent.service', 'bluechi-proxy@.service', 'bluechi-dep@.service'],
  install_dir : systemd_user_unit_dir
)
