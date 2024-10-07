#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

network_src = [
  'get_log_timestamp',
  'micros_to_millis_test',
]

foreach src : network_src
  exec_test = executable(src, src + '.c',
    link_with: [
      bluechi_lib,
    ],
    include_directories: include_directories('../../../..'),
  )
  test(src, exec_test)
endforeach
