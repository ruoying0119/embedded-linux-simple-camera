#!/bin/bash

# 嵌入式Linux相机项目构建脚本

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
LIB_DIR="$PROJECT_DIR/lib"
THIRDLIBS_DIR="$PROJECT_DIR/thirdlibs"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

print_msg() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "嵌入式Linux相机项目构建脚本"
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help        显示帮助"
    echo "  -c, --clean       清理构建"
    echo "  -d, --debug       调试模式"
    echo "  -j N              并行任务数"
    echo "  --rebuild-jpeg    强制重编译JPEG库"
    echo "  --rebuild-tslib   强制重编译tslib库"
}

clean_build() {
    print_msg "清理构建目录..."
    rm -rf "$BUILD_DIR" "$LIB_DIR" "$THIRDLIBS_DIR/jpeg-9b"
    # 只删除解压后的tslib目录，不删除压缩包
    find "$THIRDLIBS_DIR" -maxdepth 1 -type d -name "tslib*" -exec rm -rf {} \; 2>/dev/null || true
    print_msg "清理完成"
}

check_dependencies() {
    if ! command -v cmake &> /dev/null; then
        print_error "cmake未安装"
        exit 1
    fi
    
    if ! command -v make &> /dev/null; then
        print_error "make未安装"
        exit 1
    fi
    
    if [ ! -f "$THIRDLIBS_DIR/jpegsrc.v9b.tar.gz" ]; then
        print_error "JPEG库压缩包未找到，请先下载到thirdlibs目录"
        exit 1
    fi
    
    # 检查tslib压缩包（支持多种格式）
    TSLIB_FOUND="false"
    print_msg "检查tslib文件..."
    
    # 列出所有匹配的文件进行调试
    for pattern in "tslib*.tar.gz" "tslib*.tar.bz2" "tslib*.tar.xz"; do
        for archive in "$THIRDLIBS_DIR"/$pattern; do
            if [ -f "$archive" ]; then
                print_msg "找到tslib文件: $archive"
                TSLIB_FOUND="true"
                break 2
            fi
        done
    done
    
    if [ "$TSLIB_FOUND" != "true" ]; then
        print_msg "tslib文件列表:"
        ls -la "$THIRDLIBS_DIR"/tslib* 2>/dev/null || print_msg "没有找到tslib文件"
        print_error "tslib库压缩包未找到，请确保文件存在于thirdlibs目录"
        exit 1
    fi
}

build_jpeg_lib() {
    # 检查JPEG库是否已存在（除非强制重编译）
    if [ "$REBUILD_JPEG" != "true" ] && [ -f "$LIB_DIR/jpeg/lib/libjpeg.a" ]; then
        print_msg "JPEG库已存在，跳过编译（使用--rebuild-jpeg强制重编译）"
        return 0
    fi
    
    print_msg "构建JPEG静态库..."
    
    cd "$PROJECT_DIR"
    rm -rf "$THIRDLIBS_DIR/jpeg-9b"
    cd "$THIRDLIBS_DIR"
    
    if [ ! -f "jpegsrc.v9b.tar.gz" ]; then
        print_error "JPEG库压缩包未找到"
        exit 1
    fi
    
    tar -xzf jpegsrc.v9b.tar.gz
    if [ $? -ne 0 ]; then
        print_error "解压失败，请检查权限或重新下载"
        exit 1
    fi
    
    cd jpeg-9b
    
    # 检测工具链（简单检测）
    if command -v arm-buildroot-linux-gnueabihf-gcc &> /dev/null; then
        CROSS_COMPILE_PREFIX="arm-buildroot-linux-gnueabihf"
        HOST_TARGET="arm-buildroot-linux-gnueabihf"
    elif command -v arm-linux-gnueabihf-gcc &> /dev/null; then
        CROSS_COMPILE_PREFIX="arm-linux-gnueabihf"
        HOST_TARGET="arm-linux-gnueabihf"
    else
        print_error "ARM交叉编译工具链未找到，请检查CMakeLists.txt配置"
        exit 1
    fi
    
    # 配置和编译
    CC=${CROSS_COMPILE_PREFIX}-gcc \
    CXX=${CROSS_COMPILE_PREFIX}-g++ \
    AR=${CROSS_COMPILE_PREFIX}-ar \
    RANLIB=${CROSS_COMPILE_PREFIX}-ranlib \
    ./configure \
        --host=${HOST_TARGET} \
        --enable-static \
        --disable-shared \
        --prefix="$LIB_DIR/jpeg" > /dev/null
    
    if [ $? -ne 0 ]; then
        print_error "JPEG库配置失败"
        exit 1
    fi
    
    make -j$(nproc) > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        print_error "JPEG库编译失败"
        exit 1
    fi
    
    mkdir -p "$LIB_DIR"
    make install > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        print_error "JPEG库安装失败"
        exit 1
    fi
    
    print_msg "JPEG库构建完成"
    cd "$PROJECT_DIR"
}

build_tslib() {
    # 检查tslib库是否已存在（除非强制重编译）
    # tslib安装后的路径: $LIB_DIR/tslib/lib/libts.a
    if [ "$REBUILD_TSLIB" != "true" ] && [ -f "$LIB_DIR/tslib/lib/libts.a" ]; then
        print_msg "tslib库已存在，跳过编译（使用--rebuild-tslib强制重编译）"
        return 0
    fi
    
    print_msg "构建tslib静态库..."
    
    cd "$PROJECT_DIR"
    # 只删除解压后的目录，不删除压缩包
    find "$THIRDLIBS_DIR" -maxdepth 1 -type d -name "tslib*" -exec rm -rf {} \; 2>/dev/null || true
    cd "$THIRDLIBS_DIR"
    
    # 查找tslib压缩包（支持多种格式）
    TSLIB_ARCHIVE=""
    for archive in tslib*.tar.gz tslib*.tar.bz2 tslib*.tar.xz; do
        if [ -f "$archive" ]; then
            TSLIB_ARCHIVE="$archive"
            break
        fi
    done
    
    if [ -z "$TSLIB_ARCHIVE" ]; then
        print_error "tslib库压缩包未找到，请先下载到thirdlibs目录"
        exit 1
    fi
    
    print_msg "解压tslib: $TSLIB_ARCHIVE"
    
    # 根据文件扩展名选择解压方式
    case "$TSLIB_ARCHIVE" in
        *.tar.gz)  tar -xzf "$TSLIB_ARCHIVE" ;;
        *.tar.bz2) tar -xjf "$TSLIB_ARCHIVE" ;;
        *.tar.xz)  tar -xJf "$TSLIB_ARCHIVE" ;;
        *) print_error "不支持的压缩格式: $TSLIB_ARCHIVE"; exit 1 ;;
    esac
    
    if [ $? -ne 0 ]; then
        print_error "解压失败，请检查权限或重新下载"
        exit 1
    fi
    
    # 查找解压后的目录
    TSLIB_DIR=$(find . -maxdepth 1 -type d -name "tslib*" | head -1)
    if [ -z "$TSLIB_DIR" ]; then
        print_error "找不到tslib解压目录"
        exit 1
    fi
    
    cd "$TSLIB_DIR"
    print_msg "进入目录: $TSLIB_DIR"
    
    # 检测工具链
    if command -v arm-buildroot-linux-gnueabihf-gcc &> /dev/null; then
        CROSS_COMPILE_PREFIX="arm-buildroot-linux-gnueabihf"
        HOST_TARGET="arm-buildroot-linux-gnueabihf"
    elif command -v arm-linux-gnueabihf-gcc &> /dev/null; then
        CROSS_COMPILE_PREFIX="arm-linux-gnueabihf"
        HOST_TARGET="arm-linux-gnueabihf"
    else
        print_error "ARM交叉编译工具链未找到，请检查CMakeLists.txt配置"
        exit 1
    fi
    
    # 运行autogen.sh如果存在
    if [ -f "./autogen.sh" ]; then
        print_msg "运行autogen.sh..."
        ./autogen.sh > /dev/null 2>&1
    fi
    
    # 配置和编译
    print_msg "配置tslib..."
    CC=${CROSS_COMPILE_PREFIX}-gcc \
    CXX=${CROSS_COMPILE_PREFIX}-g++ \
    AR=${CROSS_COMPILE_PREFIX}-ar \
    RANLIB=${CROSS_COMPILE_PREFIX}-ranlib \
    ./configure \
        --host=${HOST_TARGET} \
        --enable-static \
        --disable-shared \
        --without-plugins \
        --prefix=/
    
    if [ $? -ne 0 ]; then
        print_error "tslib库配置失败"
        exit 1
    fi
    
    print_msg "编译tslib..."
    make -j$(nproc)
    if [ $? -ne 0 ]; then
        print_error "tslib库编译失败，查看详细错误："
        make 2>&1 | tail -20
        exit 1
    fi
    
    mkdir -p "$LIB_DIR/tslib"
    print_msg "安装tslib到临时目录..."
    
    # 尝试完整安装，如果失败则手动安装必要文件
    if ! make install DESTDIR="$LIB_DIR/tslib" > /dev/null 2>&1; then
        print_msg "完整安装失败，手动安装核心文件..."
        
        # 手动创建目录结构
        mkdir -p "$LIB_DIR/tslib/lib" "$LIB_DIR/tslib/include"
        
        # 复制静态库
        if [ -f "src/.libs/libts.a" ]; then
            cp "src/.libs/libts.a" "$LIB_DIR/tslib/lib/"
            print_msg "已复制静态库: libts.a"
        elif [ -f "src/libts.a" ]; then
            cp "src/libts.a" "$LIB_DIR/tslib/lib/"
            print_msg "已复制静态库: libts.a"
        else
            print_error "找不到tslib静态库文件"
            exit 1
        fi
        
        # 复制头文件
        if [ -f "src/tslib.h" ]; then
            cp "src/tslib.h" "$LIB_DIR/tslib/include/"
            print_msg "已复制头文件: tslib.h"
        else
            print_error "找不到tslib头文件"
            exit 1
        fi
    fi
    
    # 检查安装结果
    print_msg "检查tslib安装结果..."
    if [ -f "$LIB_DIR/tslib/lib/libts.a" ]; then
        print_msg "tslib静态库安装成功: $LIB_DIR/tslib/lib/libts.a"
    else
        print_msg "tslib安装目录内容:"
        find "$LIB_DIR/tslib" -name "*.a" 2>/dev/null || true
        find "$LIB_DIR/tslib" -name "libts*" 2>/dev/null || true
    fi
    
    print_msg "tslib库构建完成"
    cd "$PROJECT_DIR"
}

build_project() {
    local build_type=$1
    local jobs=$2
    
    print_msg "构建嵌入式相机项目..."
    
    # 构建JPEG库
    build_jpeg_lib
    
    # 构建tslib库
    build_tslib
    
    # 构建项目
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_BUILD_TYPE=$build_type ..
    if [ $? -ne 0 ]; then
        print_error "CMake配置失败"
        exit 1
    fi
    
    make -j$jobs
    if [ $? -ne 0 ]; then
        print_error "项目编译失败"
        exit 1
    fi
    
    print_msg "构建完成!"
    cd "$PROJECT_DIR"
}

# 默认参数
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || echo "4")
REBUILD_JPEG="false"
REBUILD_TSLIB="false"

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help) show_help; exit 0 ;;
        -c|--clean) clean_build; exit 0 ;;
        -d|--debug) BUILD_TYPE="Debug"; shift ;;
        -j|--jobs) JOBS="$2"; shift 2 ;;
        --rebuild-jpeg) REBUILD_JPEG="true"; shift ;;
        --rebuild-tslib) REBUILD_TSLIB="true"; shift ;;
        *) print_error "未知选项: $1"; exit 1 ;;
    esac
done

# 执行构建
print_msg "嵌入式Linux相机项目构建"
check_dependencies
build_project "$BUILD_TYPE" "$JOBS"
