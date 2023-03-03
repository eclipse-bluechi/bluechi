# SPDX-License-Identifier: GPL-2.0-or-later

# Installing the public DBus API
install_data(
    'org.containers.hirte.Job.xml',
    'org.containers.hirte.Manager.xml',
    'org.containers.hirte.Monitor.xml',
    'org.containers.hirte.Node.xml',
    install_dir: join_paths(get_option('datadir'), 'dbus-1', 'interfaces'),
)

# Installing the DBus permission configuration files
dbus_service_dir = join_paths(get_option('sysconfdir'), 'dbus-1', 'system.d')
dbus_srv_user = get_option('dbus-srv-user')
if dbus_srv_user == ''
    dbus_srv_user = 'root'
endif

conf = configuration_data()
conf.set('dbus_srv_user', dbus_srv_user)

configure_file(
    output: 'org.containers.hirte.conf',
    input: 'org.containers.hirte.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
configure_file(
    output: 'org.containers.hirte.Agent.conf',
    input: 'org.containers.hirte.Agent.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
