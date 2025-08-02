#include "jpeg_handler.h"

int jpeg_decode_file_to_lcd(const char *jpeg_path, lcd_device_t *lcd) {
    FILE *jpeg_file = NULL;
    int min_height = lcd->height, min_width = lcd->width, valid_bytes;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    // 初始化JPEG解码器
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    
    // 打开JPEG文件
    jpeg_file = fopen(jpeg_path, "rb");
    if (!jpeg_file) {
        perror("打开JPEG文件失败");
        jpeg_destroy_decompress(&cinfo);
        return ERROR;
    }
    
    // 指定解码数据源
    jpeg_stdio_src(&cinfo, jpeg_file);
    
    // 读取图像信息
    jpeg_read_header(&cinfo, TRUE);
    
    // 设置解码参数
    cinfo.out_color_space = JCS_RGB;
    
    // 开始解码
    jpeg_start_decompress(&cinfo);
    
    // 为缓冲区分配空间
    unsigned char *jpeg_line_buf = malloc(cinfo.output_components * cinfo.output_width);
    unsigned int *fb_line_buf = malloc(lcd->line_length);
    
    // 判断图像和LCD屏哪个分辨率更低
    if (cinfo.output_width < min_width)
        min_width = cinfo.output_width;
    if (cinfo.output_height < min_height)
        min_height = cinfo.output_height;
    
    // 读取数据，数据按行读取
    valid_bytes = min_width * lcd->bpp / 8;
    unsigned char *ptr = lcd->fbbase;
    
    while (cinfo.output_scanline < min_height) {
        jpeg_read_scanlines(&cinfo, &jpeg_line_buf, 1);
        
        // 将RGB888数据转换为LCD格式
        for (int i = 0; i < min_width; i++) {
            unsigned int red = jpeg_line_buf[i * 3];
            unsigned int green = jpeg_line_buf[i * 3 + 1];
            unsigned int blue = jpeg_line_buf[i * 3 + 2];
            unsigned int color = (red << 16) | (green << 8) | blue;
            fb_line_buf[i] = color;
        }
        
        memcpy(ptr, fb_line_buf, valid_bytes);
        ptr += lcd->width * lcd->bpp / 8;
    }
    
    // 完成解码
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    // 释放内存
    free(jpeg_line_buf);
    free(fb_line_buf);
    fclose(jpeg_file);
    
    return SUCCESS;
}

int jpeg_decode_data_to_lcd(const char *jpeg_data, int size, lcd_device_t *lcd) {
    int min_height = lcd->height, min_width = lcd->width, valid_bytes;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    // 初始化JPEG解码器
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    
    // 指定解码数据源为内存
    jpeg_mem_src(&cinfo, (unsigned char*)jpeg_data, size);
    
    // 读取图像信息
    jpeg_read_header(&cinfo, TRUE);
    
    // 设置解码参数
    cinfo.out_color_space = JCS_RGB;
    
    // 开始解码
    jpeg_start_decompress(&cinfo);
    
    // 为缓冲区分配空间
    unsigned char *jpeg_line_buf = malloc(cinfo.output_components * cinfo.output_width);
    unsigned int *fb_line_buf = malloc(lcd->line_length);
    
    // 判断图像和LCD屏哪个分辨率更低
    if (cinfo.output_width < min_width)
        min_width = cinfo.output_width;
    if (cinfo.output_height < min_height)
        min_height = cinfo.output_height;
    
    // 读取数据，数据按行读取
    valid_bytes = min_width * lcd->bpp / 8;
    unsigned char *ptr = lcd->fbbase;
    
    while (cinfo.output_scanline < min_height) {
        jpeg_read_scanlines(&cinfo, &jpeg_line_buf, 1);
        
        // 将RGB888数据转换为LCD格式
        for (int i = 0; i < min_width; i++) {
            unsigned int red = jpeg_line_buf[i * 3];
            unsigned int green = jpeg_line_buf[i * 3 + 1];
            unsigned int blue = jpeg_line_buf[i * 3 + 2];
            unsigned int color = (red << 16) | (green << 8) | blue;
            fb_line_buf[i] = color;
        }
        
        memcpy(ptr, fb_line_buf, valid_bytes);
        ptr += lcd->width * lcd->bpp / 8;
    }
    
    // 完成解码
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    // 释放内存
    free(jpeg_line_buf);
    free(fb_line_buf);
    
    return SUCCESS;
}

int jpeg_save_frame(const char *filename, const unsigned char *frame_data, int frame_size) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        return ERROR;
    }
    
    size_t written = fwrite(frame_data, 1, frame_size, file);
    fclose(file);
    
    if (written != frame_size) {
        return ERROR;
    }
    
    return SUCCESS;
}

int jpeg_get_info(const char *jpeg_path, jpeg_info_t *info) {
    FILE *jpeg_file = fopen(jpeg_path, "rb");
    if (!jpeg_file) {
        return ERROR;
    }
    
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpeg_file);
    jpeg_read_header(&cinfo, TRUE);
    
    info->width = cinfo.image_width;
    info->height = cinfo.image_height;
    info->components = cinfo.num_components;
    
    jpeg_destroy_decompress(&cinfo);
    fclose(jpeg_file);
    
    return SUCCESS;
}
