# SPDX-License-Identifier: GPL-2.0-or-later

network_src = [
  'get_log_timestamp',
  'get_time_seconds',
]

foreach src : network_src
  exec_test = executable(src, src + '.c',
    link_with: [
      hirte_lib,
    ],
    include_directories: include_directories('../../../..'),
  )
  test(src, exec_test)
endforeach
