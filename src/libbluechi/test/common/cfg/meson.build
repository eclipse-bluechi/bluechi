# SPDX-License-Identifier: LGPL-2.1-or-later

cfg_src = [
  'cfg_get_set_test',
  'cfg_load_complete_configuration_test',
  'cfg_load_from_env_test',
  'cfg_load_from_file_test',
]

foreach src : cfg_src
  exec_test = executable(src, src + '.c',
    link_with: [
      bluechi_lib,
    ],
    include_directories: include_directories('../../../..'),
  )
  test(src, exec_test)
endforeach
