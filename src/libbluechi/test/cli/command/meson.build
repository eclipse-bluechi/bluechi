#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

command_src = [
  'command_add_option_test',
  'command_flag_exists_test',
  'command_get_option_test',
  'command_get_option_long_test',
  'get_option_type_test',
  'command_execute_test',
  'methods_get_method_test',
]

foreach src : command_src
  exec_test = executable(src, src + '.c',
    link_with: [
      bluechi_lib,
    ],
    include_directories: include_directories('../../../..'),
  )
  test(src, exec_test)
endforeach
