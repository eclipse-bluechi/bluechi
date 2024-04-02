#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

agent_src = [
  'agent_apply_config_test',
]

# setup node test src files to include in compilation
node_test_src = []
foreach src : node_src
    # skip main to avoid duplicate main function error
    if src == 'main.c'
        continue
    endif
    node_test_src += '../../' + src
endforeach

foreach src : agent_src
  exec_test = executable(src,
    node_test_src + [src + '.c'],
    dependencies: [
        systemd_dep,
        inih_dep,
        hashmapc_dep,
    ],
    link_with: [
        bluechi_lib,
    ],
    c_args: common_cflags,
    include_directories: include_directories('../../..'),
  )
  test(src, exec_test)
endforeach
