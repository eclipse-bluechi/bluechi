# client build configuration

client_src = [
  'client.c'
]

executable(
  'hirte-client',
  client_src,
  dependencies: [
    systemd_dep,
  ],
  link_with: [
    hirte_lib,
  ],
  install: true
)
