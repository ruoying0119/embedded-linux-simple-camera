#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include "common.h"
#include <tslib.h>

// 触摸屏数据结构
typedef struct {
    struct tsdev *ts;           // tslib设备指针
    int x, y;                   // 当前触摸坐标
    int is_pressed;             // 是否按下
    pthread_mutex_t mutex;      // 互斥锁
} touchscreen_t;

// 触摸事件类型
typedef enum {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_PHOTO,          // 拍照
    TOUCH_EVENT_GALLERY,        // 打开相册
    TOUCH_EVENT_NEXT,           // 下一张
    TOUCH_EVENT_PREV,           // 上一张
    TOUCH_EVENT_RETURN          // 返回
} touch_event_t;

// 函数声明
int touchscreen_init(touchscreen_t *ts);
void touchscreen_cleanup(touchscreen_t *ts);
int touchscreen_read(touchscreen_t *ts, int *x, int *y);
touch_event_t touchscreen_get_event(touchscreen_t *ts);
void* touchscreen_thread(void *arg);

// 全局触摸屏实例
extern touchscreen_t g_touchscreen;
extern int g_touch_thread_running;

#endif // TOUCHSCREEN_H
