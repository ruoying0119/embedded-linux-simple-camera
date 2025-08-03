# 嵌入式Linux简易相机

ARM平台的简易相机项目，演示静态链接和模块化构建。

## 快速开始

```bash
# 1. 准备依赖库文件
# 请将以下文件放入 thirdlibs/ 目录：
# - jpegsrc.v9b.tar.gz
# - tslib-1.21.tar.gz (或其他版本的 .tar.gz/.tar.bz2/.tar.xz)

# 2. 修改CMakeLists.txt中的工具链路径

# 3. 构建项目
./build.sh

# 4. 部署到ARM设备


## 项目结构

```
├── src/                # 源代码模块
├── include/           # 头文件
├── thirdlibs/         # 第三方库源码包
├── background/         # 背景图片等资源
├── CMakeLists.txt     # 主构建配置
├── src/CMakeLists.txt # 模块构建配置
└── build.sh          # 自动构建脚本
```

## 技术特点

- **静态链接**：JPEG和tslib库直接编译包含，无运行时依赖
- **自动构建**：脚本自动处理第三方库编译和安装
- **模块化设计**：分离的功能模块，便于维护和扩展
- **新手友好**：详细注释和简化的构建流程

## 构建选项

```bash
./build.sh                 # 默认构建
./build.sh -d              # 调试模式
./build.sh -c              # 清理构建
./build.sh -j 4            # 指定并行任务数
./build.sh --rebuild-jpeg  # 强制重编译JPEG库
./build.sh --rebuild-tslib # 强制重编译tslib库
./build.sh -h              # 显示帮助
```

## 配置说明

**工具链配置（CMakeLists.txt）：**
```cmake
# 根据实际环境修改工具链路径
set(TOOLCHAIN_PATH "/path/to/your/toolchain/bin")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-buildroot-linux-gnueabihf-gcc")

# 如果工具链在PATH中，可以简化为：
# set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
```

**运行时配置（common.h）：**
```c
#define CAMERA_WIDTH 800           // 摄像头分辨率
#define CAMERA_HEIGHT 600
#define DEFAULT_PHOTO_DIR "/home/"  // 照片存储目录
#define BUFFER_COUNT 4             // V4L2缓冲区数量
```

## 功能模块

### 1. LCD显示模块 (`lcd.c/lcd.h`)
- Framebuffer初始化和配置
- JPEG图像显示
- 屏幕清理

### 2. 触摸屏模块 (`touchscreen.c/touchscreen.h`)
- TSLib触摸屏初始化
- 触摸事件检测和处理
- 多线程触摸读取
- 事件类型识别

### 3. V4L2摄像头模块 (`v4l2_camera.c/v4l2_camera.h`)
- V4L2设备初始化
- 摄像头格式设置
- 缓冲区管理
- 视频流控制

### 4. JPEG处理模块 (`jpeg_handler.c/jpeg_handler.h`)
- JPEG编解码
- 图像格式转换
- 文件和内存数据处理
- LCD显示适配

### 5. 相册管理模块 (`gallery.c/gallery.h`)
- 双向链表图片管理
- 目录扫描和图片加载
- 线程安全操作
- 图片浏览控制

## 硬件要求

- IMX6ULL开发板（或其他支持Linux的ARM开发板）
- USB摄像头（支持V4L2）
- LCD显示屏（支持Framebuffer）
- 电阻/电容式触摸屏

## 软件依赖

- **JPEG库** - libjpeg或libjpeg-turbo
- **TSLib库** - 触摸屏支持库
- **V4L2** - Video4Linux2驱动框架
- **pthread** - POSIX线程库

## 构建说明

### 使用CMake构建

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目
cmake ..

# 3. 编译
make -j$(nproc)

# 4. 运行
sudo ./embedded-camera
```

### 使用构建脚本

```bash
# 默认构建（Release模式）
./build.sh

# 调试模式构建
./build.sh -d

# 清理构建
./build.sh -c

# 指定并行任务数
./build.sh -j 4

# 强制重编译依赖库
./build.sh --rebuild-jpeg --rebuild-tslib

# 查看帮助
./build.sh -h
```

## 交叉编译说明

项目默认就是为ARM交叉编译设计的，通过CMakeLists.txt中的工具链配置实现：

```cmake
# 工具链配置示例
set(TOOLCHAIN_PATH "/path/to/your/toolchain/bin")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-buildroot-linux-gnueabihf-gcc")
```

直接运行 `./build.sh` 即可进行ARM交叉编译构建。

## 运行说明

1. **确保设备权限**：程序需要root权限访问framebuffer和输入设备
2. **准备背景图片**：将background1.jpg和background2.jpg放置到指定目录
3. **配置设备路径**：根据实际硬件调整设备文件路径

### 触摸区域定义

- **拍照按钮**：X(850-1000), Y(130-210)
- **相册按钮**：X(850-1000), Y(390-470)
- **下一张**：X(850-1000), Y(260-340)
- **上一张**：X(850-1000), Y(0-80)
- **返回按钮**：X(850-1000), Y(520-600)

## API文档

### LCD模块API

```c
int lcd_init(lcd_device_t *lcd);                    // 初始化LCD
void lcd_cleanup(lcd_device_t *lcd);                // 清理LCD资源
int lcd_show_jpeg_file(lcd_device_t *lcd, const char *path);  // 显示JPEG文件
int lcd_show_jpeg_data(lcd_device_t *lcd, const char *data, int size);  // 显示JPEG数据
```

### 触摸屏模块API

```c
int touchscreen_init(touchscreen_t *ts);            // 初始化触摸屏
touch_event_t touchscreen_get_event(touchscreen_t *ts);  // 获取触摸事件
void* touchscreen_thread(void *arg);                // 触摸屏线程函数
```

### 相册模块API

```c
int gallery_init(gallery_t *gallery, const char *dir);      // 初始化相册
int gallery_add_image(gallery_t *gallery, const char *file); // 添加图片
jpeg_node_t* gallery_get_next(gallery_t *gallery);          // 获取下一张
jpeg_node_t* gallery_get_prev(gallery_t *gallery);          // 获取上一张
```

## 配置选项

可在`include/common.h`中修改以下配置：

```c
#define CAMERA_WIDTH 800           // 摄像头宽度
#define CAMERA_HEIGHT 600          // 摄像头高度
#define DEFAULT_PHOTO_DIR "/home/" // 照片存储目录
#define BUFFER_COUNT 4             // V4L2缓冲区数量
```

### 常见问题

1. **权限不足**：使用sudo运行程序
2. **设备文件不存在**：检查摄像头和触摸屏设备节点
3. **库文件缺失**：安装所需的开发库
4. **交叉编译失败**：检查工具链路径和库文件

### 调试技巧

```bash
# 调试模式构建
./build.sh -d

# 强制重编译依赖库
./build.sh --rebuild-jpeg --rebuild-tslib

# 检查设备
ls /dev/video* /dev/fb* /dev/input/*
```

## 项目亮点

- **简洁架构**：专注核心功能，代码结构清晰
- **静态链接**：减少运行时依赖，适合嵌入式环境
- **自动化构建**：一键构建，新手友好
- **模块化设计**：便于学习和二次开发

本项目适合嵌入式Linux开发学习和简单相机应用开发。

## 拓展思路
1. **添加lvgl**：添加LVGL图形库，实现GUI界面
2. **yuv源数据添加h264编解码**：添加h264编解码

## 开源协议

本项目采用MIT开源协议。

## 相关链接

- [原项目技术博客](https://zhuanlan.zhihu.com/p/679857620)
- [项目演示视频](https://t.bilibili.com/890608455772012608?share_source=pc_native)


