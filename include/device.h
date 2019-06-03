#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <linux/videodev2.h>
#include <pthread.h>

enum {MJPEG=V4L2_PIX_FMT_MJPEG, YUYV=V4L2_PIX_FMT_YUYV};
//MJPEG 获取的视频是以数据流的格式---图片格式
//YUYV  获取的视频是直接以图片的格式
typedef struct
{
	unsigned char* 	driver;
	unsigned char* 	bus_info;
	unsigned char* 	card;
	int		version;
	int		width;
	int		height;
}dev_info;//摄像头的基本信息

typedef struct
{
    void* addr;
    int length;
} BufferNode;

typedef struct
{
    void* temp;
    int length;
}TempBuff;

typedef struct
{
	int		dev_fd;//--->cameraFd
	int		num_buffer;
	int		frame_size;//图片的大小 300x200x2
	int		format;//图片的格式
	int		fps;//帧频率每秒多少张图片
	int		stream_port;//sock_port

    TempBuff tempbuff;
    BufferNode* mem_list;
	dev_info  info;
	struct v4l2_buffer buf;
}dev_config;//设备配置信息

extern dev_config devconf;//the device config information

extern void			get_camera_info();
extern int			enable(int fd);//open
extern int			disable(int fd);//close
extern int			get_frame(dev_config * camera);//获取图片(帧)信息
extern int 			init_camera(const char* str, dev_config* camera);// 初始化设备
extern void 		destroy_camera(dev_config* camera);
extern void 		set_dev_config(void);
extern int			is_dev_enabled();
extern int			dev_enable();//for operation
extern int			dev_disable();//for operation
#endif
