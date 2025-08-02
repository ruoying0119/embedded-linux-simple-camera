#include "gallery.h"

// 全局相册实例
gallery_t g_gallery = {0};

int gallery_init(gallery_t *gallery, const char *photo_dir) {
    // 初始化链表头节点
    gallery->head = malloc(sizeof(jpeg_node_t));
    if (!gallery->head) {
        perror("分配内存失败");
        return ERROR;
    }
    
    strcpy(gallery->head->name, BACKGROUND2_PATH);
    gallery->head->next = gallery->head;
    gallery->head->prev = gallery->head;
    gallery->current = gallery->head;
    gallery->image_count = 0;
    
    strncpy(gallery->photo_dir, photo_dir, MAX_PATH_LEN - 1);
    gallery->photo_dir[MAX_PATH_LEN - 1] = '\0';
    
    // 初始化互斥锁
    if (pthread_mutex_init(&gallery->mutex, NULL) != 0) {
        perror("互斥锁初始化失败");
        free(gallery->head);
        return ERROR;
    }
    
    return SUCCESS;
}

void gallery_cleanup(gallery_t *gallery) {
    if (!gallery->head) return;
    
    pthread_mutex_lock(&gallery->mutex);
    
    jpeg_node_t *current = gallery->head->next;
    while (current != gallery->head) {
        jpeg_node_t *next = current->next;
        free(current);
        current = next;
    }
    
    free(gallery->head);
    gallery->head = NULL;
    
    pthread_mutex_unlock(&gallery->mutex);
    pthread_mutex_destroy(&gallery->mutex);
}

int gallery_load_images(gallery_t *gallery) {
    DIR *dp = opendir(gallery->photo_dir);
    if (!dp) {
        perror("打开目录失败");
        return ERROR;
    }
    
    struct dirent *pdir;
    int max_index = 0;
    
    pthread_mutex_lock(&gallery->mutex);
    
    while ((pdir = readdir(dp)) != NULL) {
        if (pdir->d_type == DT_REG) {  // 普通文件
            if (strstr(pdir->d_name, ".jpg")) {  // JPEG文件
                char full_path[512];  // 增加缓冲区大小
                int ret = snprintf(full_path, sizeof(full_path), "%s/%s", 
                                 gallery->photo_dir, pdir->d_name);
                
                // 检查路径是否被截断
                if (ret >= sizeof(full_path)) {
                    print_msg("路径过长，跳过文件: %s", pdir->d_name);
                    continue;
                }
                
                // 创建新节点
                jpeg_node_t *newnode = malloc(sizeof(jpeg_node_t));
                if (!newnode) {
                    continue;
                }
                
                strcpy(newnode->name, full_path);
                
                // 头插法插入链表
                jpeg_node_t *first = gallery->head->next;
                gallery->head->next = newnode;
                newnode->prev = gallery->head;
                newnode->next = first;
                first->prev = newnode;
                
                gallery->image_count++;
                
                // 计算最大索引（用于后续拍照命名）
                char name_copy[30];
                strcpy(name_copy, pdir->d_name);
                char *token = strtok(name_copy, ".");
                if (token) {
                    int index = atoi(token);
                    if (index > max_index) {
                        max_index = index;
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&gallery->mutex);
    closedir(dp);
    
    return max_index;
}

int gallery_add_image(gallery_t *gallery, const char *filename) {
    jpeg_node_t *newnode = malloc(sizeof(jpeg_node_t));
    if (!newnode) {
        return ERROR;
    }
    
    strcpy(newnode->name, filename);
    
    pthread_mutex_lock(&gallery->mutex);
    
    // 头插法
    jpeg_node_t *first = gallery->head->next;
    gallery->head->next = newnode;
    newnode->prev = gallery->head;
    newnode->next = first;
    first->prev = newnode;
    
    gallery->image_count++;
    
    pthread_mutex_unlock(&gallery->mutex);
    
    return SUCCESS;
}

jpeg_node_t* gallery_get_current(gallery_t *gallery) {
    pthread_mutex_lock(&gallery->mutex);
    jpeg_node_t *current = gallery->current;
    pthread_mutex_unlock(&gallery->mutex);
    return current;
}

jpeg_node_t* gallery_get_next(gallery_t *gallery) {
    pthread_mutex_lock(&gallery->mutex);
    gallery->current = gallery->current->next;
    if (gallery->current == gallery->head) {
        gallery->current = gallery->head->next;
    }
    jpeg_node_t *current = gallery->current;
    pthread_mutex_unlock(&gallery->mutex);
    return current;
}

jpeg_node_t* gallery_get_prev(gallery_t *gallery) {
    pthread_mutex_lock(&gallery->mutex);
    gallery->current = gallery->current->prev;
    if (gallery->current == gallery->head) {
        gallery->current = gallery->head->prev;
    }
    jpeg_node_t *current = gallery->current;
    pthread_mutex_unlock(&gallery->mutex);
    return current;
}

void gallery_print_list(gallery_t *gallery) {
    pthread_mutex_lock(&gallery->mutex);
    
    jpeg_node_t *current = gallery->head->next;
    printf("相册图片列表:\n");
    while (current != gallery->head) {
        printf("  %s\n", current->name);
        current = current->next;
    }
    printf("总计 %d 张图片\n", gallery->image_count);
    
    pthread_mutex_unlock(&gallery->mutex);
}

int gallery_get_next_image_index(gallery_t *gallery) {
    pthread_mutex_lock(&gallery->mutex);
    int max_index = gallery->image_count;
    pthread_mutex_unlock(&gallery->mutex);
    return max_index + 1;
}
