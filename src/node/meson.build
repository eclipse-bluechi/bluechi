# node build configuration

node_src = [
  'dbus.c',
  'node.c',
  'opt.c'
]
node_src += common_src

executable(
  'node',
  node_src,
  include_directories: [
    common_inc
  ],
  dependencies: [
    systemd_dep
  ],
  install: true
)
