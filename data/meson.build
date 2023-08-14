# SPDX-License-Identifier: GPL-2.0-or-later

# Installing the public DBus API
install_data(
    [
        'io.github.eclipse-bluechi.bluechi.Job.xml',
        'io.github.eclipse-bluechi.bluechi.Manager.xml',
        'io.github.eclipse-bluechi.bluechi.Monitor.xml',
        'io.github.eclipse-bluechi.bluechi.Node.xml',
        'io.github.eclipse-bluechi.bluechi.Agent.xml'
    ],
    install_dir: join_paths(get_option('datadir'), 'dbus-1', 'interfaces'), 
)

# Installing the DBus permission configuration files
dbus_service_dir = join_paths(get_option('datadir'), 'dbus-1', 'system.d')
dbus_srv_user = get_option('dbus-srv-user')
if dbus_srv_user == ''
    dbus_srv_user = 'root'
endif

conf = configuration_data()
conf.set('dbus_srv_user', dbus_srv_user)

configure_file(
    output: 'io.github.eclipse-bluechi.bluechi.conf',
    input: 'io.github.eclipse-bluechi.bluechi.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
configure_file(
    output: 'io.github.eclipse-bluechi.bluechi.Agent.conf',
    input: 'io.github.eclipse-bluechi.bluechi.Agent.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
