# orch build configuration

orch_src = [
    'orchestrator.h',
    'orchestrator.c',
    'peer-manager.h',
    'peer-manager.c',
    'main.c',
    'opt.c',
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
