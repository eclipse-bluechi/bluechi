#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

common_src = [
  'bc_log_init_test',
  'bc_log_to_stderr_full_test',
  'bc_log_to_stderr_test',
  'log_level_to_string_test',
  'log_target_to_str_test',
  'string_to_log_level_test',
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
