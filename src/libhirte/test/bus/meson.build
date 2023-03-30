# SPDX-License-Identifier: GPL-2.0-or-later

bus_src = [
  'bus_test'
]

foreach src : bus_src
  exec_test = executable(src, src + '.c',
    link_with: [
      hirte_lib,
    ],
    include_directories: include_directories('../../..'),
  )
  test(src, exec_test)
endforeach
