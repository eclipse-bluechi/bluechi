# node build configuration

node_src = [
  'main.c',
  'node.c'
]

executable(
  'hirte-node',
  node_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  include_directories: include_directories('..'),
  install: true
)
