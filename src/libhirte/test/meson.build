# SPDX-License-Identifier: GPL-2.0-or-later

# common test configuration

config_src = [
  'config_test.c'
]
exec_config_test = executable('config_test', config_src,
  link_with: [
    hirte_lib,
  ],
  include_directories: include_directories('../..'),
)
test('config_test', exec_config_test)

list_src = [
  'list_test.c'
]
exec_list_test = executable('list_test', list_src,
  link_with: [
    hirte_lib,
  ],
  include_directories: include_directories('../..'),
)
test('list_test', exec_list_test)

parse_util_src = [
  'parse-util_test.c'
]

e = executable('parse-util_test', parse_util_src,
  link_with: [
    hirte_lib,
  ],
  include_directories: include_directories('../..'),
)
test('parse-util_test', e)
