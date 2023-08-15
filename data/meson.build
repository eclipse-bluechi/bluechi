# SPDX-License-Identifier: LGPL-2.1-or-later

# Installing the public DBus API
install_data(
    [
        'org.eclipse.bluechi.Job.xml',
        'org.eclipse.bluechi.Manager.xml',
        'org.eclipse.bluechi.Monitor.xml',
        'org.eclipse.bluechi.Node.xml',
        'org.eclipse.bluechi.Agent.xml'
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
    output: 'org.eclipse.bluechi.conf',
    input: 'org.eclipse.bluechi.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
configure_file(
    output: 'org.eclipse.bluechi.Agent.conf',
    input: 'org.eclipse.bluechi.Agent.conf.in',
    configuration: conf,
    install_dir: dbus_service_dir,
)
