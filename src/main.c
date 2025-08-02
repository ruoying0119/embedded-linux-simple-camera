#include "common.h"
#include "lcd.h"
#include "touchscreen.h"
#include "v4l2_camera.h"
#include "jpeg_handler.h"
#include "gallery.h"
#include <signal.h>

// 全局变量
static int app_running = 1;
static pthread_t touch_thread;
static int signal_count = 0;  // 信号计数

// 信号处理函数
void signal_handler(int sig) {
    signal_count++;
    printf("\n收到信号 %d，退出程序... (第%d次)\n", sig, signal_count);
    
    if (signal_count == 1) {
        // 第一次信号：正常退出
        app_running = 0;
        g_touch_thread_running = 0;
    } else if (signal_count >= 2) {
        // 第二次信号：强制退出
        printf("强制退出程序\n");
        exit(1);
    }
}

// 函数声明
static int initialize_system(void);
static void cleanup_system(void);
static int take_photo(void);
static void view_gallery(void);
static void main_loop(void);

int main(void) {
    printf("嵌入式Linux简易相机启动...\n");
    printf("按 Ctrl+C 退出程序（连按两次强制退出）\n");
    
    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 系统初始化
    if (initialize_system() != SUCCESS) {
        printf("系统初始化失败\n");
        return -1;
    }
    
    // 显示背景
    lcd_show_jpeg_file(&g_lcd, BACKGROUND1_PATH);
    
    // 主循环
    main_loop();
    
    // 清理资源
    cleanup_system();
    
    printf("相机应用退出\n");
    return 0;
}

static int initialize_system(void) {
    // 初始化LCD
    if (lcd_init(&g_lcd) != SUCCESS) {
        printf("LCD初始化失败\n");
        return ERROR;
    }
    
    // 初始化触摸屏
    if (touchscreen_init(&g_touchscreen) != SUCCESS) {
        printf("触摸屏初始化失败\n");
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    // 初始化摄像头
    if (v4l2_camera_init(&g_camera, "/dev/video1") != SUCCESS) {
        printf("摄像头初始化失败\n");
        touchscreen_cleanup(&g_touchscreen);
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    // 设置摄像头格式
    if (v4l2_camera_set_format(&g_camera, CAMERA_WIDTH, CAMERA_HEIGHT, 
                               V4L2_PIX_FMT_MJPEG) != SUCCESS) {
        printf("设置摄像头格式失败\n");
        v4l2_camera_cleanup(&g_camera);
        touchscreen_cleanup(&g_touchscreen);
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    // 开始摄像头流式传输
    if (v4l2_camera_start_streaming(&g_camera) != SUCCESS) {
        printf("开启摄像头流式传输失败\n");
        v4l2_camera_cleanup(&g_camera);
        touchscreen_cleanup(&g_touchscreen);
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    // 初始化相册
    printf("初始化相册目录: %s\n", DEFAULT_PHOTO_DIR);
    if (gallery_init(&g_gallery, DEFAULT_PHOTO_DIR) != SUCCESS) {
        printf("相册初始化失败\n");
        v4l2_camera_cleanup(&g_camera);
        touchscreen_cleanup(&g_touchscreen);
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    // 加载现有图片
    int max_index = gallery_load_images(&g_gallery);
    
    // 启动触摸屏线程
    g_touch_thread_running = 1;
    if (pthread_create(&touch_thread, NULL, touchscreen_thread, &g_touchscreen) != 0) {
        printf("创建触摸屏线程失败\n");
        gallery_cleanup(&g_gallery);
        v4l2_camera_cleanup(&g_camera);
        touchscreen_cleanup(&g_touchscreen);
        lcd_cleanup(&g_lcd);
        return ERROR;
    }
    
    return SUCCESS;
}

static void cleanup_system(void) {
    printf("开始清理系统资源...\n");
    
    // 停止触摸屏线程
    g_touch_thread_running = 0;
    
    if (touch_thread) {
        printf("等待触摸屏线程退出...\n");
        
        // 等待线程退出，如果阻塞则由信号处理函数强制退出
        pthread_join(touch_thread, NULL);
        printf("触摸屏线程已退出\n");
    }
    
    // 清理各模块
    printf("清理模块资源...\n");
    gallery_cleanup(&g_gallery);
    v4l2_camera_cleanup(&g_camera);
    touchscreen_cleanup(&g_touchscreen);
    lcd_cleanup(&g_lcd);
    
    printf("系统资源清理完成\n");
}

static int take_photo(void) {
    static int photo_index = 0;
    char filename[MAX_PATH_LEN];
    
    // 生成文件名
    photo_index = gallery_get_next_image_index(&g_gallery);
    snprintf(filename, sizeof(filename), "%s/%d.jpg", DEFAULT_PHOTO_DIR, photo_index);
    
    // 获取一帧图像
    unsigned char *frame_data;
    unsigned int frame_size;
    
    if (v4l2_camera_get_frame(&g_camera, &frame_data, &frame_size) != SUCCESS) {
        printf("获取图像帧失败\n");
        return ERROR;
    }
    
    // 保存为JPEG文件
    if (jpeg_save_frame(filename, frame_data, frame_size) != SUCCESS) {
        printf("保存照片失败\n");
        v4l2_camera_put_frame(&g_camera);
        return ERROR;
    }
    
    // 添加到相册
    gallery_add_image(&g_gallery, filename);
    
    // 归还帧缓冲
    v4l2_camera_put_frame(&g_camera);
    
    sleep(1);  // 防止连续拍照
    
    return SUCCESS;
}

static void view_gallery(void) {
    // 显示背景
    lcd_show_jpeg_file(&g_lcd, BACKGROUND2_PATH);
    
    jpeg_node_t *current_image = gallery_get_current(&g_gallery);
    if (current_image && current_image != g_gallery.head) {
        lcd_show_jpeg_file(&g_lcd, current_image->name);
    }
    
    int prev_x = 0, prev_y = 0;
    
    while (app_running) {
        int current_x, current_y;
        
        pthread_mutex_lock(&g_touchscreen.mutex);
        current_x = g_touchscreen.x;
        current_y = g_touchscreen.y;
        pthread_mutex_unlock(&g_touchscreen.mutex);
        
        // 检查是否有新的触摸事件
        if (prev_x != current_x || prev_y != current_y) {
            touch_event_t event = touchscreen_get_event(&g_touchscreen);
            
            switch (event) {
                case TOUCH_EVENT_NEXT:
                    current_image = gallery_get_next(&g_gallery);
                    if (current_image && current_image != g_gallery.head) {
                        lcd_show_jpeg_file(&g_lcd, current_image->name);
                    }
                    break;
                    
                case TOUCH_EVENT_PREV:
                    current_image = gallery_get_prev(&g_gallery);
                    if (current_image && current_image != g_gallery.head) {
                        lcd_show_jpeg_file(&g_lcd, current_image->name);
                    }
                    break;
                    
                case TOUCH_EVENT_RETURN:
                    lcd_show_jpeg_file(&g_lcd, BACKGROUND1_PATH);
                    return;
                    
                default:
                    break;
            }
            
            prev_x = current_x;
            prev_y = current_y;
        }
        
        usleep(100000);  // 100ms延时
    }
}

static void main_loop(void) {
    unsigned char *frame_data;
    unsigned int frame_size;
    int prev_x = 0, prev_y = 0;
    int frame_count = 0;
    
    printf("进入主循环，显示摄像头画面...\n");
    printf("触摸屏区域: X[%d-%d], Y[%d-%d]\n", 
           TOUCH_AREA_X_MIN, TOUCH_AREA_X_MAX, 
           TOUCH_AREA_Y_MIN, TOUCH_AREA_Y_MAX);
    
    while (app_running) {
        // 获取并显示摄像头图像
        if (v4l2_camera_get_frame(&g_camera, &frame_data, &frame_size) == SUCCESS) {
            lcd_show_jpeg_data(&g_lcd, (const char*)frame_data, frame_size);
            v4l2_camera_put_frame(&g_camera);
            
            // 每100帧打印一次信息
            if (++frame_count % 100 == 0) {
                printf("已显示 %d 帧图像\n", frame_count);
            }
        }
        
        // 检查触摸事件
        int current_x, current_y;
        pthread_mutex_lock(&g_touchscreen.mutex);
        current_x = g_touchscreen.x;
        current_y = g_touchscreen.y;
        pthread_mutex_unlock(&g_touchscreen.mutex);
        
        if (prev_x != current_x || prev_y != current_y) {
            printf("检测到触摸坐标变化: (%d,%d) -> (%d,%d)\n", 
                   prev_x, prev_y, current_x, current_y);
            
            touch_event_t event = touchscreen_get_event(&g_touchscreen);
            
            switch (event) {
                case TOUCH_EVENT_PHOTO:
                    printf("拍照\n");
                    take_photo();
                    break;
                    
                case TOUCH_EVENT_GALLERY:
                    printf("查看相册\n");
                    view_gallery();
                    break;
                    
                default:
                    printf("触摸事件: %d (无对应操作)\n", event);
                    break;
            }
            
            // 重置触摸坐标
            pthread_mutex_lock(&g_touchscreen.mutex);
            g_touchscreen.x = 0;
            g_touchscreen.y = 0;
            pthread_mutex_unlock(&g_touchscreen.mutex);
            
            prev_x = current_x;
            prev_y = current_y;
        }
        
        usleep(33000);  // ~30fps
    }
    
    printf("主循环退出\n");
}
