# node build configuration

node_src = [
  'main.c',
  'node.c'
]
node_src += common_src
node_src += ini_src

executable(
  'hirte-node',
  node_src,
  include_directories: [
    common_inc
  ],
  dependencies: [
    systemd_dep
  ],
  link_with: hirte_lib,
  install: true
)
