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

# build shared library
subdir('libchihua-msg')

# build binaries
subdir('binchihua')
