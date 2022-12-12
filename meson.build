project(
  'blue-chihuahua',
  'c',
  version: '0.0.1',
  license: 'GPL-2.0-or-later',
  default_options: [
    'warning_level=3',
    'debug=true'
  ]
)

# external dependencies
systemd_dep = dependency('libsystemd')


# internal dependencies
common_inc = include_directories('src/common')
common_src = files('src/common/parse-util.c')
ini_inc = include_directories('src/ini')
ini_src = files('src/ini/ini.c')


# client
subdir('src/client')


# node
subdir('src/node')


# orch
subdir('src/orch')


# tests
subdir('test/common')
