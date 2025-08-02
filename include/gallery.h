#ifndef GALLERY_H
#define GALLERY_H

#include "common.h"

// 图片链表节点
typedef struct jpeg_node {
    char name[MAX_FILENAME_LEN];      // 图像名
    struct jpeg_node *next;           // 下一张
    struct jpeg_node *prev;           // 上一张
} jpeg_node_t;

// 相册管理结构
typedef struct {
    jpeg_node_t *head;                // 头节点
    jpeg_node_t *current;             // 当前显示的图片
    int image_count;                  // 图片总数
    char photo_dir[MAX_PATH_LEN];     // 照片目录
    pthread_mutex_t mutex;            // 互斥锁
} gallery_t;

// 函数声明
int gallery_init(gallery_t *gallery, const char *photo_dir);
void gallery_cleanup(gallery_t *gallery);
int gallery_load_images(gallery_t *gallery);
int gallery_add_image(gallery_t *gallery, const char *filename);
jpeg_node_t* gallery_get_current(gallery_t *gallery);
jpeg_node_t* gallery_get_next(gallery_t *gallery);
jpeg_node_t* gallery_get_prev(gallery_t *gallery);
void gallery_print_list(gallery_t *gallery);
int gallery_get_next_image_index(gallery_t *gallery);

// 全局相册实例
extern gallery_t g_gallery;

#endif // GALLERY_H
