# SPDX-License-Identifier: GPL-2.0-or-later

network_src = [
  'is_ipv4_test',
  'is_ipv6_test',
  'get_address_test',
  'assemble_tcp_address',
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
