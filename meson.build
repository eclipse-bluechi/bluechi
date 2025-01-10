#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

project(
  'bluechi',
  'c',
  version: run_command(join_paths(meson.current_source_dir(), 'build-scripts', 'version.sh'), ' short', check: true).stdout().strip(),
  license: 'LGPL-2.1-or-later',
  default_options: [
    'c_std=gnu17',     # Adds "-std=gnu17".  Includes GNU 17 extensions.
    'warning_level=2', # Adds "-Wextra".  Enables additional warnings.
    'debug=true',      # Adds "-g".  Object files include debugging symbols.
    'werror=true'      # Adds "-Werror".  Treat warnings as errors.
  ]
)

test_cflags = [ '-Wno-cast-function-type', '-Wno-unknown-warning-option' ]

cc = meson.get_compiler('c')
common_cflags = cc.get_supported_arguments(test_cflags)

# Link with systemd shared library.
systemd_dep = dependency('libsystemd')

# Set option to use either the session or system D-Bus
api_bus = get_option('api_bus')
if api_bus == 'user'
  common_cflags += '-DUSE_USER_API_BUS'
endif

if get_option('with_analyzer')
  add_project_arguments('-fanalyzer', language : 'c')
endif

# Set option to get coverage collection
with_coverage = get_option('with_coverage')
if with_coverage
  add_project_arguments('-coverage', language : 'c')
  add_project_arguments(
    '-fprofile-dir=' + join_paths(get_option('prefix'), get_option('localstatedir'), 'tmp', 'bluechi-coverage'),
    language : 'c')
  add_project_link_arguments('-lgcov', language : 'c')

  # Compiler should not perform any optimization when code coverage is reported, but meson RPM support sets -O2
  # into CFLAGS, so we need to override here
  common_cflags += '-Og'

  # Coverage source code mapping files `*.gcno` are created as a part of the build for each `*.c` file, but it's
  # very hard to tell meson to install files, which are created as a side effect from the build, so it's easier
  # to use custom install script
  meson.add_install_script('build-scripts/install-coverage.sh')
endif

# Build time configuration header file
conf = configuration_data()

# Enable GNU extensions.
conf.set('_GNU_SOURCE', true)

with_selinux = get_option('with_selinux')
if with_selinux
  selinux_dep = dependency('libselinux', required: true)
  conf.set('CONFIG_H_USE_SELINUX', true)
endif

prefixdir = get_option('prefix')

conf.set_quoted('CONFIG_H_UDS_SOCKET_PATH', get_option('unix_domain_socket_path'))
conf.set_quoted('CONFIG_H_SYSCONFDIR', join_paths(prefixdir, get_option('sysconfdir')))
conf.set_quoted('CONFIG_H_DATADIR', join_paths(prefixdir, get_option('datadir')))
conf.set_quoted('CONFIG_H_BC_VERSION', run_command(
    join_paths(meson.current_source_dir(), 'build-scripts', 'version.sh'), ' long', check: true
  ).stdout().strip()
)

config_h = configure_file(
        output : 'config.h',
        configuration : conf)

# For lint to work properly the same include needs to be added manually to clang-tidy arguments
add_project_arguments('-include', 'config.h', language : 'c')


# (External) subprojects used by bluechi
inih = subproject('inih', default_options: [
  'default_library=static',           # include it as static library
  '-Dinih:distro_install=false',      # disable distro install so other options are used
  '-Dinih:max_line_length=520',       # set maximum line length so that ~500 characters can be used as value
  '-Dinih:stop_on_first_error=true',  # abort after first error when parsing file
])
inih_dep = inih.get_variable('inih_dep')

hashmapc = subproject('hashmap.c', default_options: [
  'default_library=static', # include it as static library
])
hashmapc_dep = hashmapc.get_variable('hashmapc_dep')


# Subdirectory for the shared library.
subdir('src/libbluechi')

# Configuration files
subdir('config')

# selinux
if with_selinux
  subdir('selinux')
endif

# build each binary
subdir('src/controller')
subdir('src/agent')
subdir('src/client')
subdir('src/proxy')
subdir('src/is-online')

# Subdirectory for the API description
subdir('data')

# Subdirectory for the man documentation
subdir('doc/man')

# systemd unit files
subdir('systemd-units')

# add custom clang-tidy target since default auto-generated ninja target doesn't support passing compilation options
# overrides the auto-generated ninja target clang-tidy
clangtidy = find_program('clang-tidy', required : false)
if clangtidy.found()
  run_target('clang-tidy', command : ['build-scripts/clang-tidy-all.sh'])
  run_target('clang-tidy-fix', command : ['build-scripts/clang-tidy-all.sh', '--fix'])
endif
