# 指定使用 CMake 的最低版本号
cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(myweb C CXX)

# 添加搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/src/Base
    ${PROJECT_SOURCE_DIR}/src/Net
    ${PROJECT_SOURCE_DIR}/src/Poller
    ${PROJECT_SOURCE_DIR}/src/Http
    ${PROJECT_SOURCE_DIR}/src/Timer
    ${PROJECT_SOURCE_DIR}/src/Logger
    ${PROJECT_SOURCE_DIR}/src/Memory
    ${PROJECT_SOURCE_DIR}/src/Mysql
    )

# 包含这些目录下的.cpp文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Net SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Poller SRC_POLLER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Http SRC_HTTP)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Timer SRC_TIMER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Logger SRC_LOG)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Memory SRC_MEMORY)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Mysql SRC_MYSQL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 生成动态库 tiny_network
add_library(myweb SHARED 
            ${SRC_BASE}
            ${SRC_NET}
            ${SRC_POLLER}
            ${SRC_TIMER}
            ${SRC_LOG}
            ${SRC_HTTP}
            ${SRC_MEMORY}
            ${SRC_MYSQL}
            )

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(myweb pthread mysqlclient)

# 设置生成动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置安装的默认路径
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# 将目录安装到 include/tiny_network 下（只安装头文件）
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include/myweb_work
        FILES_MATCHING PATTERN "*.h")

# # 加载http
add_subdirectory(src/Http/test)

# add_subdirectory(src/logger/test)

# add_subdirectory(src/memory/test)

# add_subdirectory(src/Mysql/test)

# # 加载base
# add_subdirectory(src/base/test)
# add_subdirectory(main/)