# node build configuration

node_src = [
  'main.c',
  'agent.c'
]

executable(
  'hirte-agent',
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
