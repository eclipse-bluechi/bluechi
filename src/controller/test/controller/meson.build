#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

controller_src = [
  'controller_apply_config_test',
]

# setup controller test src files to include in compilation
controller_test_src = []
foreach src : ctrl_src
    # skip main to avoid duplicate main function error
    if src == 'main.c'
        continue
    endif
    controller_test_src += '../../' + src
endforeach

foreach src : controller_src
  exec_test = executable(src,
    controller_test_src + [src + '.c'],
    dependencies: controller_deps,
    link_with: [
        bluechi_lib,
    ],
    c_args: common_cflags,
    include_directories: include_directories('../../..'),
  )
  test(src, exec_test)
endforeach
