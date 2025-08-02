cmake_minimum_required(VERSION 3.10)
project(embedded-camera C)

# ARM交叉编译配置
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ========================================
# 工具链配置 - 请根据实际环境修改以下路径
# ========================================
set(TOOLCHAIN_PATH "/home/zq/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-buildroot-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-buildroot-linux-gnueabihf-g++")

# 如果工具链在PATH中，可以简化为：
# set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
# set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

# 强制静态链接
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
# 注意：不使用 -static 来避免libdl链接问题
set(CMAKE_EXE_LINKER_FLAGS "-pthread")

# 项目配置
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")

# 包含目录
include_directories(include)

# JPEG库配置 - 只链接JPEG库演示静态链接
set(JPEG_ROOT ${CMAKE_SOURCE_DIR}/lib/jpeg)
find_library(JPEG_LIBRARY 
    NAMES jpeg
    PATHS ${JPEG_ROOT}/lib
    NO_DEFAULT_PATH
    REQUIRED
)
include_directories(${JPEG_ROOT}/include)

# tslib库配置 - 静态链接
# tslib使用DESTDIR安装，路径为: lib/tslib/lib/ 和 lib/tslib/include/
set(TSLIB_ROOT ${CMAKE_SOURCE_DIR}/lib/tslib)
find_library(TSLIB_LIBRARY 
    NAMES ts
    PATHS ${TSLIB_ROOT}/lib
    NO_DEFAULT_PATH
    REQUIRED
)
include_directories(${TSLIB_ROOT}/include)

# 添加子目录
add_subdirectory(src)

# 查找pthread库
find_package(Threads REQUIRED)

# 创建主可执行文件
add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} 
    camera_modules
    ${JPEG_LIBRARY}
    ${TSLIB_LIBRARY}
    Threads::Threads
    dl  # tslib需要动态库加载函数
)

message(STATUS "构建配置: ARM静态链接")
message(STATUS "编译器: ${CMAKE_C_COMPILER}")
message(STATUS "JPEG库: ${JPEG_LIBRARY}")
message(STATUS "tslib库: ${TSLIB_LIBRARY}")
