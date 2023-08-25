# SPDX-License-Identifier: GPL-2.0-or-later

cfg_src = [
  'cfg_get_set_test',
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
