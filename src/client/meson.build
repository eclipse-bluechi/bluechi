# client build configuration

client_src = [
  'main.c',
  'client.c',
  'printer.c'
]

executable(
  'hirtectl',
  client_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  c_args: common_cflags,
  install: true,
  include_directories: include_directories('..')
)
