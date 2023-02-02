libhirte_src = [
    'bus/peer-bus.c',
    'bus/peer-bus.h',
    'bus/systemd-bus.c',
    'bus/systemd-bus.h',
    'bus/user-bus.c',
    'bus/user-bus.h',
    'bus/system-bus.c',
    'bus/system-bus.h',
    'common/common.h',
    'common/list.h',
    'common/opt.h',
    'common/parse-util.c',
    'common/parse-util.h',
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
]

hirte_lib = static_library('hirte',
                            libhirte_src,
                            dependencies: [
                                systemd_dep
                            ],
                            include_directories: include_directories('..'))


# build test binaries
subdir('test')
