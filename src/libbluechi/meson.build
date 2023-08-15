# SPDX-License-Identifier: LGPL-2.1-or-later

libbluechi_src = [
    'bus/bus.c',
    'bus/bus.h',
    'bus/utils.c',
    'bus/utils.h',
    'common/cfg.c',
    'common/cfg.h',
    'common/common.h',
    'common/event-util.c',
    'common/event-util.h',
    'common/protocol.c',
    'common/list.h',
    'common/opt.h',
    'common/parse-util.c',
    'common/parse-util.h',
    'common/network.h',
    'common/network.c',
    'common/time-util.c',
    'common/time-util.h',
    'service/service.c',
    'service/service.h',
    'service/shutdown.c',
    'service/shutdown.h',
    'socket.c',
    'socket.h',
    'log/log.h',
    'log/log.c',
]

bluechi_lib = static_library('bluechi',
                            libbluechi_src,
                            dependencies: [
                                systemd_dep,
                                inih_dep,
                                hashmapc_dep,
                            ],
                            c_args: common_cflags,
                            include_directories: include_directories('..'))


# build test binaries
subdir('test')
