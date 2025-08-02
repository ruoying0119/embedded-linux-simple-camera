#include "v4l2_camera.h"

// 全局摄像头实例
v4l2_camera_t g_camera = {0};

int v4l2_camera_init(v4l2_camera_t *camera, const char *device_path) {
    // 打开摄像头设备
    camera->fd = open(device_path, O_RDWR);
    if (camera->fd < 0) {
        perror("打开摄像头设备失败");
        return ERROR;
    }
    
    camera->width = CAMERA_WIDTH;
    camera->height = CAMERA_HEIGHT;
    camera->pixel_format = V4L2_PIX_FMT_MJPEG;
    camera->buffer_count = BUFFER_COUNT;
    camera->streaming = 0;
    
    return SUCCESS;
}

void v4l2_camera_cleanup(v4l2_camera_t *camera) {
    if (camera->streaming) {
        v4l2_camera_stop_streaming(camera);
    }
    
    // 取消内存映射
    for (int i = 0; i < camera->buffer_count; i++) {
        if (camera->buffers[i]) {
            munmap(camera->buffers[i], camera->buffer_lengths[i]);
            camera->buffers[i] = NULL;
        }
    }
    
    if (camera->fd >= 0) {
        close(camera->fd);
        camera->fd = -1;
    }
}

int v4l2_camera_set_format(v4l2_camera_t *camera, int width, int height, unsigned int format) {
    struct v4l2_format vfmt;
    
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vfmt.fmt.pix.width = width;
    vfmt.fmt.pix.height = height;
    vfmt.fmt.pix.pixelformat = format;
    
    int ret = ioctl(camera->fd, VIDIOC_S_FMT, &vfmt);
    if (ret < 0) {
        perror("设置采集格式错误");
        return ERROR;
    }
    
    // 验证设置的格式
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(camera->fd, VIDIOC_G_FMT, &vfmt);
    if (ret < 0) {
        perror("读取采集格式失败");
        return ERROR;
    }
    
    camera->width = vfmt.fmt.pix.width;
    camera->height = vfmt.fmt.pix.height;
    camera->pixel_format = vfmt.fmt.pix.pixelformat;
    
    // 申请缓冲队列
    struct v4l2_requestbuffers reqbuffer;
    reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.count = camera->buffer_count;
    reqbuffer.memory = V4L2_MEMORY_MMAP;
    
    ret = ioctl(camera->fd, VIDIOC_REQBUFS, &reqbuffer);
    if (ret < 0) {
        perror("申请缓冲队列失败");
        return ERROR;
    }
    
    // 映射缓冲区
    struct v4l2_buffer mapbuffer;
    mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    for (int i = 0; i < camera->buffer_count; i++) {
        mapbuffer.index = i;
        ret = ioctl(camera->fd, VIDIOC_QUERYBUF, &mapbuffer);
        if (ret < 0) {
            perror("查询缓存队列失败");
            return ERROR;
        }
        
        camera->buffers[i] = (unsigned char *)mmap(NULL, mapbuffer.length, 
                                                   PROT_READ | PROT_WRITE, 
                                                   MAP_SHARED, camera->fd, 
                                                   mapbuffer.m.offset);
        camera->buffer_lengths[i] = mapbuffer.length;
        
        // 将缓冲区放入队列
        ret = ioctl(camera->fd, VIDIOC_QBUF, &mapbuffer);
        if (ret < 0) {
            perror("放入队列失败");
            return ERROR;
        }
    }
    
    return SUCCESS;
}

int v4l2_camera_start_streaming(v4l2_camera_t *camera) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(camera->fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        perror("开启流式传输失败");
        return ERROR;
    }
    
    camera->streaming = 1;
    return SUCCESS;
}

int v4l2_camera_stop_streaming(v4l2_camera_t *camera) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(camera->fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        perror("关闭流式传输失败");
        return ERROR;
    }
    
    camera->streaming = 0;
    return SUCCESS;
}

static struct v4l2_buffer current_buffer;

int v4l2_camera_get_frame(v4l2_camera_t *camera, unsigned char **data, unsigned int *length) {
    current_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(camera->fd, VIDIOC_DQBUF, &current_buffer);
    if (ret < 0) {
        perror("获取帧数据失败");
        return ERROR;
    }
    
    *data = camera->buffers[current_buffer.index];
    *length = current_buffer.bytesused;
    
    return SUCCESS;
}

int v4l2_camera_put_frame(v4l2_camera_t *camera) {
    int ret = ioctl(camera->fd, VIDIOC_QBUF, &current_buffer);
    if (ret < 0) {
        perror("归还帧缓冲失败");
        return ERROR;
    }
    
    return SUCCESS;
}
