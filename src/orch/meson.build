# orch build configuration

orch_src = [
  'orch.c',
  'opt.c',
  'controller.c'
]
orch_src += ini_src
orch_src += common_src

executable(
  'orch',
  orch_src,
  include_directories: [
    common_inc,
    ini_inc
  ],
  dependencies: [
    systemd_dep
  ],
  link_with: blue_chihuahua_shared,
  install: true
)
