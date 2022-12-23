project(
  'blue-chihuahua',
  'c',
  version: '0.0.1',
  license: 'GPL-2.0-or-later',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'debug=true'
  ]
)

# add global arguments for gcc
add_global_arguments('-D_GNU_SOURCE', language : 'c')

# external dependencies
systemd_dep = dependency('libsystemd')

# internal dependencies
common_inc = include_directories('src/common')
common_src = files('src/common/parse-util.c')
ini_inc = include_directories('src/ini')
ini_src = files('src/ini/ini.c')

# build shared library
subdir('src/libchihua-msg')

# build binaries
subdir('src/client')
subdir('src/node')
subdir('src/orch')

# build test binaries
subdir('test/common')
