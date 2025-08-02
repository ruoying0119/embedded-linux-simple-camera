#ifndef JPEG_HANDLER_H
#define JPEG_HANDLER_H

#include "common.h"
#include "lcd.h"
#include <jpeglib.h>

// JPEG处理相关结构
typedef struct jpeg_info {
    int width;
    int height;
    int components;
    unsigned char *data;
    size_t data_size;
} jpeg_info_t;

// 函数声明
int jpeg_decode_file_to_lcd(const char *jpeg_path, lcd_device_t *lcd);
int jpeg_decode_data_to_lcd(const char *jpeg_data, int size, lcd_device_t *lcd);
int jpeg_save_frame(const char *filename, const unsigned char *frame_data, int frame_size);
int jpeg_get_info(const char *jpeg_path, jpeg_info_t *info);

#endif // JPEG_HANDLER_H
