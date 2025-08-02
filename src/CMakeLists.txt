# src目录的CMakeLists.txt

# 收集所有源文件（除了main.c）
set(MODULE_SOURCES
    lcd.c
    touchscreen.c
    v4l2_camera.c
    jpeg_handler.c
    gallery.c
)

# 创建相机模块静态库
add_library(camera_modules STATIC ${MODULE_SOURCES})

# 设置库输出位置
set_target_properties(camera_modules PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
