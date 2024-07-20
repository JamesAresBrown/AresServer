cmake_minimum_required(VERSION 3.10)
project(AresServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果需要兼容 C 语言，则添加以下选项
enable_language(C)
# 为动态库添加源文件
# 设置动态库的名称
set(LIB_NAME MyLibrary14)

# 获取 src 目录下的所有 .cpp 文件
#file(GLOB LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp" "src/**/*.cpp")

include_directories(
        src/connection
        src/include
        src/process/include
        src/cache
)

# 创建动态库
add_library(${LIB_NAME} SHARED ${LIB_SOURCES})

set(EXTERNAL_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/process/external_libs")
set(EXTERNAL_LIBS
        ${EXTERNAL_LIBS_PATH}/libdpmvs5.so
        ${EXTERNAL_LIBS_PATH}/libredis++.so.1
        ${EXTERNAL_LIBS_PATH}/libhiredis.so.1.1.0
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
        -pthread
        -lmysqlclient
        ${EXTERNAL_LIBS}
)