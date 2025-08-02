#include "touchscreen.h"
#include <tslib.h>
#include <errno.h>
#include <string.h>

// 全局触摸屏实例
touchscreen_t g_touchscreen = {0};
int g_touch_thread_running = 0;

int touchscreen_init(touchscreen_t *ts) {
    // 使用tslib初始化触摸屏
    ts->ts = ts_setup(NULL, 0);  // 阻塞模式打开
    if (ts->ts == NULL) {
        printf("错误: 无法初始化触摸屏设备\n");
        perror("ts_setup failed");
        return ERROR;
    }
    
    ts->x = 0;
    ts->y = 0;
    ts->is_pressed = 0;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&ts->mutex, NULL) != 0) {
        perror("互斥锁初始化失败");
        ts_close(ts->ts);
        return ERROR;
    }
    
    return SUCCESS;
}

void touchscreen_cleanup(touchscreen_t *ts) {
    if (ts->ts != NULL) {
        ts_close(ts->ts);
        ts->ts = NULL;
    }
    pthread_mutex_destroy(&ts->mutex);
}
int touchscreen_read(touchscreen_t *ts, int *x, int *y) {
    struct ts_sample samp;
    
    // 使用tslib读取触摸屏数据
    if (ts_read(ts->ts, &samp, 1) < 0) {
        return ERROR;
    }
    
    // 更新坐标
    pthread_mutex_lock(&ts->mutex);
    ts->x = samp.x;
    ts->y = samp.y;
    ts->is_pressed = samp.pressure > 0;
    *x = samp.x;
    *y = samp.y;
    pthread_mutex_unlock(&ts->mutex);
    
    return SUCCESS;
}

touch_event_t touchscreen_get_event(touchscreen_t *ts) {
    int x, y;
    
    pthread_mutex_lock(&ts->mutex);
    x = ts->x;
    y = ts->y;
    pthread_mutex_unlock(&ts->mutex);
    
    // 检查是否在有效触摸区域内
    if (x < TOUCH_AREA_X_MIN || x > TOUCH_AREA_X_MAX || 
        y < TOUCH_AREA_Y_MIN || y > TOUCH_AREA_Y_MAX) {
        return TOUCH_EVENT_NONE;
    }
    
    // 判断具体按钮
    if (y >= PHOTO_BUTTON_Y_MIN && y <= PHOTO_BUTTON_Y_MAX) {
        return TOUCH_EVENT_PHOTO;
    } else if (y >= GALLERY_BUTTON_Y_MIN && y <= GALLERY_BUTTON_Y_MAX) {
        return TOUCH_EVENT_GALLERY;
    } else if (y >= NEXT_BUTTON_Y_MIN && y <= NEXT_BUTTON_Y_MAX) {
        return TOUCH_EVENT_NEXT;
    } else if (y >= PREV_BUTTON_Y_MIN && y <= PREV_BUTTON_Y_MAX) {
        return TOUCH_EVENT_PREV;
    } else if (y >= RETURN_BUTTON_Y_MIN && y <= RETURN_BUTTON_Y_MAX) {
        return TOUCH_EVENT_RETURN;
    }
    
    return TOUCH_EVENT_NONE;
}

void* touchscreen_thread(void *arg) {
    touchscreen_t *ts = (touchscreen_t *)arg;
    int x, y;
    
    while (g_touch_thread_running) {
        int result = touchscreen_read(ts, &x, &y);
        if (result == SUCCESS) {
            // 只在有有效触摸时处理事件
            touch_event_t event = touchscreen_get_event(ts);
            if (event != TOUCH_EVENT_NONE) {
                // 可以在这里添加事件处理逻辑
            }
        }
        
        usleep(50000);  // 50ms延时
    }
    
    return NULL;
}
