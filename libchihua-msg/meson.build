libchihua_src = [
    'include/common/memory.h',
    'include/node.h',
    'include/orchestrator.h',
    'include/orchestrator/connection-server.h',
    'include/orchestrator/peer-manager.h',
    'include/peer-bus.h',

    'src/common/memory.h',
    'src/common/dbus.h',
    'src/node.c',
    'src/orchestrator.c',
    'src/orchestrator/connection-server.c',
    'src/orchestrator/peer-manager.c',
    'src/peer-bus.c',
]

# We use libtool-version numbers because it's easier to understand.
# Before making a release, the libruntime_so_*
# numbers should be modified. The components are of the form C:R:A.
# a) If binary compatibility has been broken (eg removed or changed interfaces)
#    change to C+1:0:0.
# b) If interfaces have been changed or added, but binary compatibility has
#    been preserved, change to C+1:0:A+1
# c) If the interface is the same as the previous version, change to C:R+1:A
libruntime_lt_c=1
libruntime_lt_r=0
libruntime_lt_a=0

libruntime_so_version = '@0@.@1@.@2@'.format((libruntime_lt_c - libruntime_lt_a),
                                              libruntime_lt_a,
                                              libruntime_lt_r)

blue_chihuahua_shared = shared_library('-chihua-msg',
                            libchihua_src,
                            dependencies: [
                                systemd_dep
                            ],
                            version : libruntime_so_version,
                            install : true)


# build test binaries
subdir('test')
