#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <jpeglib.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <pthread.h>
#include <tslib.h>
#include <stdlib.h>
#include <dirent.h>
#include <semaphore.h>
#include <sys/time.h>
int fd_fb;
int screen_size;//屏幕像素大小
int LCD_width;//LCD宽度
int LCD_height;//LCD高度

//int pressure = 0;

unsigned char *fbbase = NULL;//LCD显存地址
unsigned long line_length;       //LCD一行的长度（字节为单位）
unsigned int bpp;    //像素深度bpp


int fd;
int fd_v4l2;
int read_x=0, read_y=0;
int start_read_flag = 1;
int start_xiangce_flag = 1;

struct tsdev *ts = NULL;
int image_count = 0;
const char *background1 = "/home/Picture/background1.jpg";
const char *background2 = "/home/Picture/background2.jpg";

struct jpeg_node *image_list = NULL;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
//sem_t sem_id;



struct jpeg_node{
	char name[30];				//图像名
	struct jpeg_node *next; //下一张
	struct jpeg_node *pre;	//上一张
};

//初始化链表
struct jpeg_node *jpeg_list_Init(void)
{
	struct jpeg_node* jpeg_head = malloc(sizeof(struct jpeg_node));
	//jpeg_head ->name = NULL;
	strcpy(jpeg_head->name, background2);
	jpeg_head ->pre = jpeg_head;
	jpeg_head ->next = jpeg_head;
	return jpeg_head;
}
//插入一个新节点
void jpeg_list_insert(struct jpeg_node *jpeg_list, char *name)
{
	struct jpeg_node *newnode = malloc(sizeof(struct jpeg_node));
	//newnode->name = name;
	strcpy(newnode->name, name);
	struct jpeg_node *p = jpeg_list->next;
	//头插法
	jpeg_list->next = newnode;
	newnode->pre = jpeg_list;
	newnode->next = p;
	p->pre = newnode;
	printf("放入链表成功\n");
	p = NULL;
	//free(p);
}
//销毁链表
/*
void jpeg_list_destory(struct jpeg_node *jpeg_list)
{
	struct jpeg_node* pnext = jpeg_list->next;
	while(pnext != jpeg_list)
	{
		struct jpeg_node *q = pnext->next;
		free(pnext);
		pnext = q;
	}
	free(pnext);
	q = NULL;
	pnext = NULL;
}
*/
//遍历链表
void jpeg_list_printf(void)
{
	struct jpeg_node* pnext = image_list->next;
	while(pnext != image_list)
	{
		printf("%s\n", pnext->name);
		pnext = pnext->next;
	}
	pnext = NULL;
}

//初始化触摸屏
int Touch_screen_Init(void)
{
	ts = ts_setup(NULL, 0);//以阻塞打开
	if(NULL == ts)
	{
		perror("触摸屏初始化失败");
	}
	return 0;
}

//初始化LCD
int LCD_Init(void)
{
	struct fb_var_screeninfo var;   /* Current var */
	struct fb_fix_screeninfo fix;   /* Current fix */
	fd_fb = open("/dev/fb0", O_RDWR);
	if(fd_fb < 0)
	{
		perror("打开LCD失败");
		return -1;
	}
	//获取LCD信息
	ioctl(fd_fb, FBIOGET_VSCREENINFO, &var);//获取屏幕可变信息
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);//获取屏幕固定信息
	//LCD_width  = var.xres * var.bits_per_pixel / 8;
    //pixel_width = var.bits_per_pixel / 8;
    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	LCD_width = var.xres;
	LCD_height = var.yres;
	bpp = var.bits_per_pixel;
	line_length = fix.line_length;
	printf("LCD分辨率：%d %d\n",LCD_width, LCD_height);
	printf("bpp: %d\n", bpp);
	fbbase = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);//映射
	if (fbbase == (unsigned char *)-1)
    {
        printf("can't mmap\n");
        return -1;
    }
    memset(fbbase, 0xFF, screen_size);//LCD设置为白色背景
    
    return 0;
}

#if 0
int isOutOf500ms(struct timeval *ptPreTime, struct timeval *ptNowTime)
{
	int iPreMs;
	int iNowMs;
	
	iPreMs = ptPreTime->tv_sec * 1000 + ptPreTime->tv_usec / 1000;
	iNowMs = ptNowTime->tv_sec * 1000 + ptNowTime->tv_usec / 1000; //秒*1000=毫秒；微秒/1000=毫秒
	printf("yanshi\n");
	return (iNowMs > iPreMs + 300);
}
#endif
int read_touchscreen(int *x, int *y)
{
	struct ts_sample samp;
	//struct timeval tPreTime;
	
		if (ts_read(ts, &samp, 1) < 0) 
		{
			perror("ts_read error");
			ts_close(ts);
			return -1;
		}
		//if(isOutOf500ms(&tPreTime, &samp.tv))
		//{
		//	tPreTime = samp.tv;
				*x = samp.x;
				*y = samp.y;
			printf("anxia : %d %d", samp.x, samp.y);
		//}
		#if 0
		if(samp.pressure > 0)
		{
			if(pressure){
				*x = samp.x;
				*y = samp.y;
			}
			pressure = samp.pressure;
		}
		#endif
		//else
			//break;	
	return 0;
}
	

int LCD_JPEG_Show(const char *JpegData, int size)
{
	int min_hight = LCD_height, min_width = LCD_width, valid_bytes;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);//错误处理对象与解压对象绑定
	//创建解码对象
	jpeg_create_decompress(&cinfo);
	//指定解码数据源
	jpeg_mem_src(&cinfo, JpegData, size);
	//读取图像信息
	jpeg_read_header(&cinfo, TRUE);
	//printf("jpeg图像的大小为：%d*%d\n", cinfo.image_width, cinfo.image_height);
	//设置解码参数
	cinfo.out_color_space = JCS_RGB;//可以不设置默认为RGB
	//cinfo.scale_num = 1;
	//cinfo.scale_denom = 1;设置图像缩放，scale_num/scale_denom缩放比例，默认为1
	//开始解码
	jpeg_start_decompress(&cinfo);
	
	//为缓冲区分配空间
	unsigned char*jpeg_line_buf = malloc(cinfo.output_components * cinfo.output_width);
	unsigned int*fb_line_buf = malloc(line_length);//每个成员4个字节和RGB888对应
	//判断图像和LCD屏那个分辨率更低
	if(cinfo.output_width < min_width)
		min_width = cinfo.output_width;
	if(cinfo.output_height < min_hight)
		min_hight = cinfo.output_height;
	//读取数据，数据按行读取
	valid_bytes = min_width * bpp / 8;//一行的有效字节数，实际写进LCD显存的一行数据大小
	unsigned char *ptr = fbbase;
	while(cinfo.output_scanline < min_hight)
	{
		jpeg_read_scanlines(&cinfo, &jpeg_line_buf, 1);//每次读取一行
		//将读取到的BGR888数据转化为RGB888
		unsigned int red, green, blue;
		unsigned int color;  
		for(int i = 0; i < min_width; i++)
		{
			red = jpeg_line_buf[i*3];
			green = jpeg_line_buf[i*3+1];
			blue = jpeg_line_buf[i*3+2];
			color = red<<16 | green << 8 | blue;
			fb_line_buf[i] = color;
		}
		memcpy(ptr, fb_line_buf, valid_bytes);
		ptr += LCD_width*bpp/8;
	}
	//完成解码
	jpeg_finish_decompress(&cinfo);
	//销毁解码对象
	jpeg_destroy_decompress(&cinfo);
	//释放内存
	free(jpeg_line_buf);
	free(fb_line_buf);
	return 1;
}

//指定JPEG文件显示在LCD上,传入jpeg文件路径
int LCD_Show_JPEG(const char *jpeg_path)
{
	FILE *jpeg_file = NULL;
	int min_hight = LCD_height, min_width = LCD_width, valid_bytes;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);//错误处理对象与解压对象绑定
	//打开.jpeg图像文件
	jpeg_file = fopen(jpeg_path, "r");	//只读方式打开
	//创建解码对象
	jpeg_create_decompress(&cinfo);
	//指定解码数据源
	jpeg_stdio_src(&cinfo, jpeg_file);
	//读取图像信息
	jpeg_read_header(&cinfo, TRUE);
	//printf("jpeg图像的大小为：%d*%d\n", cinfo.image_width, cinfo.image_height);
	//设置解码参数
	cinfo.out_color_space = JCS_RGB;//可以不设置默认为RGB
	//cinfo.scale_num = 1;
	//cinfo.scale_denom = 1;设置图像缩放，scale_num/scale_denom缩放比例，默认为1
	//开始解码
	jpeg_start_decompress(&cinfo);
	
	//为缓冲区分配空间
	unsigned char*jpeg_line_buf = malloc(cinfo.output_components * cinfo.output_width);
	unsigned int*fb_line_buf = malloc(line_length);//每个成员4个字节和RGB888对应
	//判断图像和LCD屏那个分辨率更低
	if(cinfo.output_width < min_width)
		min_width = cinfo.output_width;
	if(cinfo.output_height < min_hight)
		min_hight = cinfo.output_height;
	//读取数据，数据按行读取
	valid_bytes = min_width * bpp / 8;//一行的有效字节数，实际写进LCD显存的一行数据大小
	unsigned char *ptr = fbbase;
	while(cinfo.output_scanline < min_hight)
	{
		jpeg_read_scanlines(&cinfo, &jpeg_line_buf, 1);//每次读取一行
		//将读取到的BGR888数据转化为RGB888
		unsigned int red, green, blue;
		unsigned int color;  
		for(int i = 0; i < min_width; i++)
		{
			red = jpeg_line_buf[i*3];
			green = jpeg_line_buf[i*3+1];
			blue = jpeg_line_buf[i*3+2];
			color = red<<16 | green << 8 | blue;
			fb_line_buf[i] = color;
		}
		memcpy(ptr, fb_line_buf, valid_bytes);
		ptr += LCD_width*bpp/8;
	}
	//完成解码
	jpeg_finish_decompress(&cinfo);
	//销毁解码对象
	jpeg_destroy_decompress(&cinfo);
	//释放内存
	free(jpeg_line_buf);
	free(fb_line_buf);
	return 1;
}

//把照片放入相册，image_count为目前照片名称最大索引
int xiangce_Init(void)
{
	image_list = jpeg_list_Init();
	DIR *dp = opendir("/home/");	//打开home目录
	struct dirent *pdir;
	char *temp = NULL;
	char name[15];
	//char newname[20];
	int total = 0;
	//遍历目录， 当遍历结束时返回NULL
	while(pdir = readdir(dp))	
	{
		if(pdir->d_type == DT_REG)				//判断是否为普通文件
		{
			if(strstr(pdir->d_name, ".jpg"))	//判断是否为jpg文件
			{
				char newname[20] = {0};
				sprintf(newname,"/home/%s", pdir->d_name);
				printf("%s ", pdir->d_name);
				jpeg_list_insert(image_list, newname);//将该文件名称插入链表中
				printf("%s\n", newname);
				bzero(name,15);
				strcpy(name, pdir->d_name);
				temp = strtok(name, ".");
				total = atoi(temp) > total ? atoi(temp) : total;
				printf("%d\n", total);
			}
			printf("1\n");
		}
		printf("2\n");
	}
	temp = NULL;
	return total;
}
//线程函数，开始读取屏幕坐标

void *start_read(void *arg)
{
	int x = 0, y =0;
	while(start_read_flag)
	{
		//sem_wait(&sem_id);
		printf("线程\n");
		read_touchscreen(&x, &y);
		//printf("anxia : %d %d\n", x, y);
		//usleep(500000);
		//拍照
		//sleep(1);
		if(x  > 800 && x < 1000 && y > 0 && y < 600)
		{
			printf("chuli\n");
			pthread_mutex_lock(&mutex);
			read_x = x;
 			read_y = y;
			
			pthread_mutex_unlock(&mutex);
			//printf("readx = %d, ready = %d", read_x, read_y);
		}
		printf("readx = %d, ready = %d", read_x, read_y);
		
	}
	return NULL;
}

//打开相册
void start_xiangce(void)
{

		printf("dakaixiangce\n");	
		struct jpeg_node *curr_image = image_list; //->next;//指向第一张图片    
		LCD_Show_JPEG(background2);
		LCD_Show_JPEG(curr_image->name);
		int pre_x = 0, pre_y = 0;
		while(1)
		{
			//printf("循环相册\n");
			//read_touchscreen(&touch_x, &touch_y);
			
			if(pre_x != read_x && pre_y != read_y)
			{
				if(read_x>850 && read_x<1000 && read_y>260 &&read_y<340)	//下一张
				{
					printf("下一张\n");
					curr_image = curr_image->next;
					printf("current image name :%s\n", curr_image->name);
				}
				if(read_x>850 && read_x<1000 && read_y>0 && read_y<80)	//上一张
				{
					curr_image = curr_image->pre;
					printf("上一张\n");
					printf("current image name :%s\n", curr_image->name);
				}
			}
			pre_x = read_x;
			pre_y = read_y;
			if(curr_image == image_list)
				curr_image = image_list->next;
			LCD_Show_JPEG(curr_image->name);	
			if(read_x>850 && read_x<1000 && read_y>520 && read_y<600)	//返回
			{
				LCD_Show_JPEG(background1);
				printf("返回\n");
				break;
			}
			#if 0
			pthread_mutex_lock(&mutex);
			read_x = 0;
			read_y = 0;
			pthread_mutex_unlock(&mutex);
			#endif
		}
}

int main(void)
{
	Touch_screen_Init();
	LCD_Init();
	//V4l2_Init();
	int fd = open("/dev/video1", O_RDWR);
	if(fd < 0)
	{
		perror("打开设备失败");
		return -1;
	}
	LCD_Show_JPEG(background1);
	image_count = xiangce_Init();
	//sem_init(&sem_id, 0, 1);
	jpeg_list_printf();
	printf("返回的image%d\n", image_count);
	//char buffer[800*600]
	//LCD_JPEG_Show(const char *JpegData, int size)
	//2.设置摄像头采集格式
	struct v4l2_format vfmt;
	vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	//选择视频抓取
	vfmt.fmt.pix.width = 800;//设置宽
	vfmt.fmt.pix.height = 600;//设置高
	vfmt.fmt.pix.pixelformat =  V4L2_PIX_FMT_MJPEG;//设置视频采集像素格式

	int ret = ioctl(fd, VIDIOC_S_FMT, &vfmt);// VIDIOC_S_FMT:设置捕获格式
	if(ret < 0)
	{
		perror("设置采集格式错误");
	}
	memset(&vfmt, 0, sizeof(vfmt));
	vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_G_FMT, &vfmt);	
	if(ret < 0)
	{
		perror("读取采集格式失败");
	}
	printf("设置分辨率width = %d\n", vfmt.fmt.pix.width);
	printf("设置分辨率height = %d\n", vfmt.fmt.pix.height);
	unsigned char *p = (unsigned char*)&vfmt.fmt.pix.pixelformat;
	printf("pixelformat = %c%c%c%c\n", p[0],p[1],p[2],p[3]);	

	//pthread_join(pthread_read, NULL);
	//4.申请缓冲队列
	printf("2\n");
	struct v4l2_requestbuffers reqbuffer;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.count = 4;	//申请4个缓冲区
	reqbuffer.memory = V4L2_MEMORY_MMAP;	//采用内存映射的方式

    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuffer);
	if(ret < 0)
	{
		perror("申请缓冲队列失败");
	}
	
	//映射，映射之前需要查询缓存信息->每个缓冲区逐个映射->将缓冲区放入队列
	struct v4l2_buffer mapbuffer;
	unsigned char *mmpaddr[4];//用于存储映射后的首地址
	unsigned int addr_length[4];//存储映射后空间的大小
	mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//初始化type
	for(int i = 0; i < 4; i++)
	{
		mapbuffer.index = i;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer);	//查询缓存信息
		if(ret < 0)
			perror("查询缓存队列失败");
		mmpaddr[i] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.offset);//mapbuffer.m.offset映射文件的偏移量
		addr_length[i] = mapbuffer.length;
		//放入队列
		ret = ioctl(fd, VIDIOC_QBUF, &mapbuffer);
		if(ret < 0)
			perror("放入队列失败");
	}

	//打开设备
	//int read_x, read_y;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if(ret < 0)
	{
		perror("打开设备失败");
	}
	//unsigned char rgbdata[LCD_height*LCD_width*3];//存储解码后的RGB数据
	
	//pthread_mutex_init(&mutex, NULL);
	pthread_t pthread_read;
	pthread_create(&pthread_read, NULL, start_read, NULL);
	//pthread_t pthread_start;
	//pthread_create(&pthread_start, NULL, start_xiangce, NULL);
	while(1)
	{
		//从队列中提取一帧数据
		struct v4l2_buffer readbuffer;
		readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(fd, VIDIOC_DQBUF, &readbuffer);//从缓冲队列获取一帧数据（出队列）
		//出队列后得到缓存的索引index,得到对应缓存映射的地址mmpaddr[readbuffer.index]
		if(ret < 0)
			perror("获取数据失败");
		//printf("jiankong\n");
		//read_touchscreen(&read_x, &read_y); 
		if(read_x > 850 && read_x < 1000 && read_y > 0 && read_y < 600)
		{
			
			if(read_x > 850 && read_x < 1000 && read_y > 130 && read_y < 210)
			{
				printf("paizhao\n");
				char newname[20] = {0};
				image_count++;
				sprintf(newname,"/home/%d.jpg", image_count);
				//printf("new_image:%s %d\n", newname, image_count);
				FILE *file = fopen(newname, "w+");//建立文件用于保存一帧数据
				if(NULL == file)
					printf("拍照失败");
				int res_w = fwrite(mmpaddr[readbuffer.index], readbuffer.length, 1, file);
				fclose(file);
				#if 0
				int fd_w;
				fd_w = open("newname", O_RDWR|O_CREAT, 666);
				if(0 > fd_w)
					perror("paizhaoshibai\n");
				read(fd_w, mmpaddr[readbuffer.index], readbuffer.length);
				printf("new_image:%s %d\n", newname, image_count);
				close(fd_w);
				#endif
				jpeg_list_insert(image_list, newname);
				printf("new_image:%s %d\n", newname, image_count);
				sleep(1);
			}
			else if(read_x  > 850 && read_x < 1000 && read_y > 390 && read_y < 470)
			{
				start_xiangce();				
			}
			//printf("rex:%d rex:%d", read_x, read_y);
			pthread_mutex_lock(&mutex);
			read_x = 0;
			read_y = 0;
			pthread_mutex_unlock(&mutex);
			//pthread_cond_signal(&cond);

		}
		//打开相册
		//显示在LCD上
		LCD_JPEG_Show(mmpaddr[readbuffer.index], readbuffer.length);
		//读取数据后将缓冲区放入队列
		ret = ioctl(fd, VIDIOC_QBUF, &readbuffer);
		if(ret < 0)
			perror("放入队列失败");
	}
	//关闭设备
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if(ret < 0)
		perror("关闭设备失败");
	//取消映射
	for(int i = 0; i < 4; i++)
		munmap(mmpaddr[i], addr_length[i]);

	start_read_flag = 0;	//结束线程
	start_xiangce_flag = 0;
	ts_close(ts);
	close(fd);
	close(fd_fb);
	return 0;
	
}



