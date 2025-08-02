#ifndef V4L2_CAMERA_H
#define V4L2_CAMERA_H

#include "common.h"
#include <linux/videodev2.h>

// 摄像头设备结构
typedef struct {
    int fd;                     // 设备文件描述符
    int width;                  // 图像宽度
    int height;                 // 图像高度
    unsigned int pixel_format;  // 像素格式
    int buffer_count;           // 缓冲区数量
    unsigned char *buffers[BUFFER_COUNT];  // 缓冲区地址
    unsigned int buffer_lengths[BUFFER_COUNT];  // 缓冲区长度
    int streaming;              // 是否正在流式传输
} v4l2_camera_t;

// 函数声明
int v4l2_camera_init(v4l2_camera_t *camera, const char *device_path);
void v4l2_camera_cleanup(v4l2_camera_t *camera);
int v4l2_camera_set_format(v4l2_camera_t *camera, int width, int height, unsigned int format);
int v4l2_camera_start_streaming(v4l2_camera_t *camera);
int v4l2_camera_stop_streaming(v4l2_camera_t *camera);
int v4l2_camera_get_frame(v4l2_camera_t *camera, unsigned char **data, unsigned int *length);
int v4l2_camera_put_frame(v4l2_camera_t *camera);

// 全局摄像头实例
extern v4l2_camera_t g_camera;

#endif // V4L2_CAMERA_H
