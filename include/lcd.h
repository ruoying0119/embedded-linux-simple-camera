#ifndef LCD_H
#define LCD_H

#include "common.h"
#include <linux/fb.h>

// LCD相关数据结构
typedef struct {
    int fd_fb;                  // framebuffer文件描述符
    int screen_size;            // 屏幕像素大小
    int width;                  // LCD宽度
    int height;                 // LCD高度
    unsigned char *fbbase;      // LCD显存地址
    unsigned long line_length;  // LCD一行的长度（字节为单位）
    unsigned int bpp;           // 像素深度
} lcd_device_t;

// 函数声明
int lcd_init(lcd_device_t *lcd);
void lcd_cleanup(lcd_device_t *lcd);
int lcd_clear_screen(lcd_device_t *lcd, unsigned int color);
int lcd_show_jpeg_file(lcd_device_t *lcd, const char *jpeg_path);
int lcd_show_jpeg_data(lcd_device_t *lcd, const char *jpeg_data, int size);

// 全局LCD设备实例
extern lcd_device_t g_lcd;

#endif // LCD_H
