#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dirent.h>
#include <semaphore.h>
#include <sys/time.h>

// 全局常量定义
#define MAX_FILENAME_LEN 30
#define MAX_PATH_LEN 256
#define DEFAULT_PHOTO_DIR "/home/"
#define BACKGROUND1_PATH "/home/Picture/background1.jpg"
#define BACKGROUND2_PATH "/home/Picture/background2.jpg"

// 摄像头参数
#define CAMERA_WIDTH 800
#define CAMERA_HEIGHT 600
#define BUFFER_COUNT 4

// 触摸区域定义
#define TOUCH_AREA_X_MIN 800
#define TOUCH_AREA_X_MAX 1000
#define TOUCH_AREA_Y_MIN 0
#define TOUCH_AREA_Y_MAX 600

#define PHOTO_BUTTON_Y_MIN 130
#define PHOTO_BUTTON_Y_MAX 210

#define GALLERY_BUTTON_Y_MIN 390
#define GALLERY_BUTTON_Y_MAX 470

#define NEXT_BUTTON_Y_MIN 260
#define NEXT_BUTTON_Y_MAX 340

#define PREV_BUTTON_Y_MIN 0
#define PREV_BUTTON_Y_MAX 80

#define RETURN_BUTTON_Y_MIN 520
#define RETURN_BUTTON_Y_MAX 600

// 返回值定义
#define SUCCESS 0
#define ERROR -1

#endif // COMMON_H
