#include "lcd.h"
#include "jpeg_handler.h"

// 全局LCD设备实例
lcd_device_t g_lcd = {0};

int lcd_init(lcd_device_t *lcd) {
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    
    // 打开framebuffer设备
    lcd->fd_fb = open("/dev/fb0", O_RDWR);
    if (lcd->fd_fb < 0) {
        perror("打开LCD失败");
        return ERROR;
    }
    
    // 获取LCD信息
    if (ioctl(lcd->fd_fb, FBIOGET_VSCREENINFO, &var) < 0) {
        perror("获取屏幕可变信息失败");
        close(lcd->fd_fb);
        return ERROR;
    }
    
    if (ioctl(lcd->fd_fb, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("获取屏幕固定信息失败");
        close(lcd->fd_fb);
        return ERROR;
    }
    
    // 设置LCD参数
    lcd->screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
    lcd->width = var.xres;
    lcd->height = var.yres;
    lcd->bpp = var.bits_per_pixel;
    lcd->line_length = fix.line_length;
    
    // 映射framebuffer到内存
    lcd->fbbase = mmap(NULL, lcd->screen_size, PROT_READ | PROT_WRITE, 
                       MAP_SHARED, lcd->fd_fb, 0);
    if (lcd->fbbase == (unsigned char *)-1) {
        perror("内存映射失败");
        close(lcd->fd_fb);
        return ERROR;
    }
    
    // 清屏为白色
    memset(lcd->fbbase, 0xFF, lcd->screen_size);
    
    return SUCCESS;
}

void lcd_cleanup(lcd_device_t *lcd) {
    if (lcd->fbbase && lcd->fbbase != (unsigned char *)-1) {
        munmap(lcd->fbbase, lcd->screen_size);
        lcd->fbbase = NULL;
    }
    
    if (lcd->fd_fb >= 0) {
        close(lcd->fd_fb);
        lcd->fd_fb = -1;
    }
}

int lcd_clear_screen(lcd_device_t *lcd, unsigned int color) {
    if (!lcd || !lcd->fbbase) {
        return ERROR;
    }
    
    unsigned int *pixel = (unsigned int *)lcd->fbbase;
    int total_pixels = lcd->width * lcd->height;
    
    for (int i = 0; i < total_pixels; i++) {
        pixel[i] = color;
    }
    
    return SUCCESS;
}

int lcd_show_jpeg_file(lcd_device_t *lcd, const char *jpeg_path) {
    return jpeg_decode_file_to_lcd(jpeg_path, lcd);
}

int lcd_show_jpeg_data(lcd_device_t *lcd, const char *jpeg_data, int size) {
    return jpeg_decode_data_to_lcd(jpeg_data, size, lcd);
}
