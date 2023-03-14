# SPDX-License-Identifier: GPL-2.0-or-later

project(
  'hirte',
  'c',
  version: '0.1.0',
  license: 'GPL-2.0-or-later',
  default_options: [
    'c_std=gnu17',     # Adds "-std=gnu17".  Includes GNU 17 extensions.
    'warning_level=2', # Adds "-Wextra".  Enables additional warnings.
    'debug=true',      # Adds "-g".  Object files include debugging symbols.
    'werror=true'      # Adds "-Werror".  Treat warnings as errors.
  ]
)

test_cflags = [ '-Wno-cast-function-type' ]

cc = meson.get_compiler('c')
common_cflags = cc.get_supported_arguments(test_cflags)

# Link with systemd shared library.
systemd_dep = dependency('libsystemd')

api_bus = get_option('api_bus')
if api_bus == 'user'
   common_cflags += '-DUSE_USER_API_BUS'
endif

# Build time configuration header file
conf = configuration_data()

# Enable GNU extensions.
conf.set('_GNU_SOURCE', true)

prefixdir = get_option('prefix')

conf.set_quoted('CONFIG_H_SYSCONFDIR', join_paths(prefixdir, get_option('sysconfdir')))
conf.set_quoted('CONFIG_H_DATADIR', join_paths(prefixdir, get_option('datadir')))

config_h = configure_file(
        output : 'config.h',
        configuration : conf)

# For lint to work properly the same include needs to be added manually to clang-tidy arguments
add_project_arguments('-include', 'config.h', language : 'c')


# Subdirectory for the shared library.
subdir('src/libhirte')

# Configuration files
subdir('config')

# build each binary
subdir('src/manager')
subdir('src/agent')
subdir('src/client')

# Subdirectory for the API description
subdir('data')

# Subdirectory for the man documentation
subdir('doc/man')

# systemd unit files
subdir('systemd-units')
