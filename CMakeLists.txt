cmake_minimum_required(VERSION 3.10)
project(AresServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果需要兼容 C 语言，则添加以下选项
enable_language(C)

# 创建动态库
include_directories(
        src/connection
        src/include
        src/process/include
        src/cache
)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp" "src/**/*.cpp")
add_library(MyLibrary14 SHARED ${LIB_SOURCES})

link_directories(
        /usr/local/lib/ # redis相关库文件地址
        ${CMAKE_CURRENT_SOURCE_DIR}/src/process/external_libs
)
set(EXTERNAL_LIBS
        libdpmvs5.so
        libredis++.so
        libhiredis.so
        -lhiredis
        -lredis++
        -pthread
        -lmysqlclient
)

# 如果有其他依赖库，可以在此添加链接
# target_link_libraries(MyLibrary PUBLIC SomeOtherLibrary)

# 添加可执行文件
add_executable(Server14 server.cpp)

# 添加链接库
target_link_libraries(Server14 PRIVATE
        MyLibrary14
        ${EXTERNAL_LIBS}
)