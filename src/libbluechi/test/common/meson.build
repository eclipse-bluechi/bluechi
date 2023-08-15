# SPDX-License-Identifier: LGPL-2.1-or-later

common_src = [
  'list_test',
  'parse-util_test',
  'string-util_test',
  'time-util_test'
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

subdir('cfg')
subdir('network')
subdir('time-util')
