# orch build configuration

orch_src = [
    'manager.h',
    'manager.c',
    'node.h',
    'node.c',
    'main.c',
]

executable(
  'hirte',
  orch_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  install: true,
  include_directories: include_directories('..')
)
