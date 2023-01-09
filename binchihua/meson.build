# internal dependencies
common_inc = include_directories('src/common')
common_src = files('src/common/parse-util.c', 'src/common/hashmap/hashmap.c')
ini_inc = include_directories('src/ini')
ini_src = files('src/ini/ini.c', 'src/ini/config.c')

# build each binary
subdir('src/client')
subdir('src/node')
subdir('src/orch')

# build test binaries
subdir('test/common')
