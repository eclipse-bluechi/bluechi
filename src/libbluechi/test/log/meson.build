# SPDX-License-Identifier: LGPL-2.1-or-later

common_src = [
  'string_to_log_level_test',
  'bc_log_init_test',
]

foreach src : common_src
  exec_test = executable(src, src + '.c',
    link_with: [
      bluechi_lib,
    ],
    include_directories: include_directories('../../..'),
  )
  test(src, exec_test)
endforeach
