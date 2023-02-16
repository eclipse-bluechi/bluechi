libhirte_src = [
    'bus/bus.c',
    'bus/bus.h',
    'bus/utils.c',
    'bus/utils.h',
    'common/common.h',
    'common/event-util.c',
    'common/event-util.h',
    'common/protocol.c',
    'common/list.h',
    'common/opt.h',
    'common/util.c',
    'common/util.h',
    'hashmap/hashmap.c',
    'hashmap/hashmap.h',
    'ini/config.c',
    'ini/config.h',
    'ini/ini.c',
    'ini/ini.h',
    'service/service.c',
    'service/service.h',
    'service/shutdown.c',
    'service/shutdown.h',
    'socket.c',
    'socket.h',
    'log/log.h',
    'log/log.c',
]

hirte_lib = static_library('hirte',
                            libhirte_src,
                            dependencies: [
                                systemd_dep
                            ],
                            c_args: common_cflags,
                            include_directories: include_directories('..'))


# build test binaries
subdir('test')
