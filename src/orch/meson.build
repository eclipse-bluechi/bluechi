# orch build configuration

orch_src = [
    'manager.h',
    'manager.c',
    'managednode.h',
    'managednode.c',
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
