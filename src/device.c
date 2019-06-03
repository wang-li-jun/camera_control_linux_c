#include "device.h"
#include "print.h"

#include <linux/videodev2.h>
#include <memory.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

static int is_enabled = 0;
dev_config devconf;

/*set configuration*/
extern void set_dev_config(void)
{
    devconf.info.width = 320;
    devconf.info.height = 240;
    devconf.frame_size = devconf.info.width * devconf.info.height * 2;
    devconf.fps = 30;
    devconf.format = YUYV;
    devconf.num_buffer = 4;
}

/*show supported frame interval*/
static int enum_frame_intervals(int dev, __u32 pixfmt, __u32 width, __u32 height)
{
    int ret;
    struct v4l2_frmivalenum fival;

    memset(&fival, 0, sizeof(fival));
    fival.index = 0;
    fival.pixel_format = pixfmt;
    fival.width = width;
    fival.height = height;
    printf("\tTime interval between frame: ");
    while ((ret = ioctl(dev, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
        if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                printf("%u/%u, ",
                        fival.discrete.numerator, fival.discrete.denominator); //Êä³ö·ÖÊý
        } else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
                printf("{min { %u/%u } .. max { %u/%u } }, ",
                        fival.stepwise.min.numerator, fival.stepwise.min.numerator,
                        fival.stepwise.max.denominator, fival.stepwise.max.denominator);
                break;
        } else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                printf("{min { %u/%u } .. max { %u/%u } / "
                        "stepsize { %u/%u } }, ",
                        fival.stepwise.min.numerator, fival.stepwise.min.denominator,
                        fival.stepwise.max.numerator, fival.stepwise.max.denominator,
                        fival.stepwise.step.numerator, fival.stepwise.step.denominator);
                break;
        }
        fival.index++;
    }
    printf("\n");
    if (ret != 0 && errno != EINVAL) {
        printf("ERROR enumerating frame intervals: %d\n", errno);
        return errno;
    }

    return 0;
}

/*show supported frame format*/
static int enum_frame_sizes(int dev, __u32 pixfmt)
{
    int ret;
    struct v4l2_frmsizeenum fsize;

    memset(&fsize, 0, sizeof(fsize));
    fsize.index = 0;
    fsize.pixel_format = pixfmt;
    while ((ret = ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0) {
        if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            printf("{ discrete: width = %u, height = %u }\n",
                    fsize.discrete.width, fsize.discrete.height);
            ret = enum_frame_intervals(dev, pixfmt,
                    fsize.discrete.width, fsize.discrete.height);  //²éÕÒÉè±¸Ö§³ÖµÄ Ö¡µÄ¼ä¸ôÊ±¼ä
            if (ret != 0)
                printf("  Unable to enumerate frame sizes.\n");
        } else if (fsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
            printf("{ continuous: min { width = %u, height = %u } .. "
                    "max { width = %u, height = %u } }\n",
                    fsize.stepwise.min_width, fsize.stepwise.min_height,
                    fsize.stepwise.max_width, fsize.stepwise.max_height);
            printf("  Refusing to enumerate frame intervals.\n");
            break;
        } else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
            printf("{ stepwise: min { width = %u, height = %u } .. "
                    "max { width = %u, height = %u } / "
                    "stepsize { width = %u, height = %u } }\n",
                    fsize.stepwise.min_width, fsize.stepwise.min_height,
                    fsize.stepwise.max_width, fsize.stepwise.max_height,
                    fsize.stepwise.step_width, fsize.stepwise.step_height);
            printf("  Refusing to enumerate frame intervals.\n");
            break;
        }
        fsize.index++;
    }
    if (ret != 0 && errno != EINVAL) {
        printf("ERROR enumerating frame sizes: %d\n", errno);
        return errno;
    }
    return 0;
}

void get_camera_info()
{
	int usb_camera_fd;
	char *usb_video_dev = "/dev/video0";
	struct v4l2_capability capability;
	struct v4l2_cropcap cropcap;
	struct v4l2_fmtdesc fmtdesc;
	int ret;

	usb_camera_fd = open(usb_video_dev,O_RDWR|O_NONBLOCK);
	if(usb_camera_fd == -1)
	{
		fprintf(stderr,"Can't open device %s\n",usb_video_dev);
		return;
	}

	printf("[CAPABILITY]\n");
	if (ioctl(usb_camera_fd,VIDIOC_QUERYCAP,&capability) == -1)
	{
		perror("Couldn't get videodevice capability");
		return;
	}
	else
	{
		printf("video capabilities : %x\n",capability.capabilities);// struct video_capability video_caps;
		printf("video_bus_info :%s\n",capability.bus_info);
		printf("video_card : %s\n",capability.card);
		printf("video driver : %s\n",capability.driver);
		printf("video version : %d\n",capability.version);
		if ((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
		{
			printf("Device supports capture.\n");
		}
		if ((capability.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
		{
			printf("Device supports streaming.\n");
		}
	}
	printf("\n");

	/*
	printf("[CROP CAPABILITY]\n");
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(usb_camera_fd, VIDIOC_CROPCAP, &cropcap) == -1) {
	    perror ("Couldn't get crop capability");
	}else{
		printf("video width : %d\n",cropcap.defrect.width);
		printf("video height : %d\n",cropcap.defrect.height);
	}
	printf("\n");
	*/

	printf("[SUPPORTED FORMAT]\n");
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(usb_camera_fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
	{
		fmtdesc.index++;
		printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",
				fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF,
				(fmtdesc.pixelformat >> 16) & 0xFF,
				(fmtdesc.pixelformat >> 24) & 0xFF, fmtdesc.description);
		ret = enum_frame_sizes(usb_camera_fd, fmtdesc.pixelformat);
		if (ret != 0)
		     printf("Unable to enumerate frame sizes.\n");
	}
	if (errno != EINVAL) {
		printf("Couldn't get enumerating frame formats: %d\n", errno);
	}
	printf("\n");
}


/*设置帧格式*/
static int set_frame_size(dev_config* camera)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = camera->info.width;
    fmt.fmt.pix.height = camera->info.height;
    fmt.fmt.pix.pixelformat = camera->format;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    //fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(camera->dev_fd,VIDIOC_S_FMT, &fmt) < 0) {
        fprintf(stderr, "ioctl error:set_frame_size:%s\n", strerror(errno));
        return 0;
    }
    return 1;
}

/*设置帧频率*/
static int set_frame_fp(int fps, int fd)
{
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(struct v4l2_streamparm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps;
    if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
        fprintf(stderr, "ioctl error:set_frame_fp:%s\n", strerror(errno));
        return 0;
    }
    return 1;
}

/*为内核里面的帧缓存分配内存*/
static int set_req_buff(dev_config* camera)
{
    if (camera->num_buffer < 0) {
        camera->num_buffer = 4;
    }
    struct v4l2_requestbuffers reqbuffers;
    memset(&reqbuffers, 0, sizeof(struct v4l2_requestbuffers));
    //采用mmap从内存中获得内核空间的帧缓存的地址
    //形成内存映射
    reqbuffers.count = camera->num_buffer;
    reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffers.memory = V4L2_MEMORY_MMAP;
    if (ioctl(camera->dev_fd, VIDIOC_REQBUFS, &reqbuffers) < 0) {
        fprintf(stderr, "ioctl error:set_req_buff:%s\n", strerror(errno));
        return 0;
    }
    return 1;
}

/*获取记录缓存的物理空间, 内存映射*/
static int map_dev_buff(dev_config* camera)
{
	int i;

    camera->mem_list = (BufferNode*)calloc(camera->num_buffer, sizeof(BufferNode));
    if (camera->mem_list==NULL) {
        fprintf(stderr, "calloc error:map_dev_buff:%s\n", strerror(errno));
        return 0;
    }

    for (i = 0; i < camera->num_buffer; i++) {
        memset(&camera->buf, 0, sizeof(struct v4l2_buffer));
        camera->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        camera->buf.memory = V4L2_MEMORY_MMAP;
        camera->buf.index = i;

        //get the buffer
		if (ioctl(camera->dev_fd, VIDIOC_QUERYBUF, &camera->buf) < 0) {
			fprintf(stderr, "ioctl error:map_dev_buff:%s\n", strerror(errno));
			return 0;
		}

		//转换成相对地址(内核空间映射到用户空间)
		camera->mem_list[i].addr=mmap(NULL, camera->buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, camera->dev_fd, camera->buf.m.offset);
		if (camera->mem_list[i].addr == MAP_FAILED) {
			fprintf(stderr, "mmap error:map_dev_buff in round %d:%s\n", i, strerror(errno));
			return 0;
		}
		camera->mem_list[i].length = camera->buf.length;
    }

    //temp buffer allocation
    //temp buffer allocation
    switch (camera->format)
    {
		case V4L2_PIX_FMT_MJPEG:
		   printf("Not support MJPEG\n");
		   return 0;
		   break;
		case V4L2_PIX_FMT_YUYV:
		   camera->tempbuff.length = camera->buf.length/2*3;
		   camera->tempbuff.temp = calloc(1, camera->tempbuff.length);
		   if (camera->tempbuff.temp == NULL) {
				fprintf(stderr, "calloc temp buffer error:%s\n", strerror(errno));
				return 0;
		   }
		   camera->frame_size = camera->tempbuff.length;
		   printf("buffer length:%d\n", camera->buf.length);
		   printf("temp buffer length:%d\n", camera->tempbuff.length);
		   break;
		default:
		   printf("Invalid video format setting\n");
		   return 0;
		   break;
    }

    //放入缓存队列
    for (i = 0; i < camera->num_buffer; i++) {
    	memset(&camera->buf, 0, sizeof(struct v4l2_buffer));
		camera->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		camera->buf.memory = V4L2_MEMORY_MMAP;
		camera->buf.index = i;
		if (ioctl(camera->dev_fd, VIDIOC_QBUF, &camera->buf) < -1) {
			fprintf(stderr, "ioctl error:map_dev_buff:VIDIOC_QBUF in round %d:%s\n", i, strerror(errno));
			return 0;
		}
    }
    return 1;
}

/*初始化设备数据*/
static int init_dev(dev_config *camera)
{
    /*设置帧格式*/
    if (!set_frame_size(camera)) {
        return 0;
    }
    printf("Set frame size successful\n");

    /*设置帧频率*/
    if (!set_frame_fp(camera->fps, camera->dev_fd)) {
        return 0;
    }
    printf("Set frame rate successful\n");

    /*设置内核空间中的帧缓存（分配内存）*/
    if (!set_req_buff(camera)) {
        return 0;
    }
    printf("Request buffer successful\n");

    /*获取并记录缓存的物理空间*/
    if (!map_dev_buff(camera)) {
        return 0;
    }
    printf("Map device buffer successful\n");
    return 1;
}

/*初始化设备*/
int init_camera(const char* str, dev_config* camera)
{
    /*open the cameraFd device*/
    int cameraFd;
    cameraFd=open(str, O_RDWR|O_NONBLOCK);
    //cameraFd=open(str, O_RDWR);
    if (cameraFd == -1) {
        fprintf(stderr, "init_camera error:open:%s\n", strerror(errno));
        return 0;
    }
    printf("Open camera device successful\n");
    camera->dev_fd = cameraFd;
    return init_dev(camera);
}

/*销毁设备资源*/
void destroy_camera(dev_config* camera)
{
    if (camera == NULL) {
        return;
    }
    if (camera->dev_fd < 0) {
        return;
    }
    disable(camera->dev_fd);//close the camera
    if (camera->mem_list != NULL) {
    	int i=0;
    	for (; i<camera->num_buffer; i++) {
            munmap(camera->mem_list[i].addr, camera->mem_list[i].length);
        }
        free(camera->mem_list);
        camera->mem_list = NULL;
    }

    if (camera->tempbuff.temp != NULL) {
        free(camera->tempbuff.temp);
        camera->tempbuff.temp = NULL;
    }

    if (camera->dev_fd > 2) {
        close(camera->dev_fd);
        camera->dev_fd = -1;
    }
}

int disable(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        fprintf(stderr, "ioctl error:disable:%s\n", strerror(errno));
        return 0;
    }
    is_enabled = 0;
    return 1;
}

int enable(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        fprintf(stderr, "ioctl error:enable:%s\n", strerror(errno));
        return 0;
    }
    is_enabled = 1;
    return 1;
}

int is_dev_enabled()
{
	if(is_enabled){
		return 1;
	}else{
		return 0;
	}
}

int dev_enable()
{
	is_enabled = 1;
	return 1;
}

int dev_disable()
{
	is_enabled = 0;
	return 1;
}

int get_frame(dev_config* camera)
{
    memset(&camera->buf, 0, sizeof(struct v4l2_buffer));
    camera->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera->buf.memory = V4L2_MEMORY_MMAP;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(camera->dev_fd, &fds);
    struct timeval t;
    t.tv_sec = 1;
    t.tv_usec = 0;
    int s = 0;
    /*采集数据*/
    while (1) {
    	if(!is_dev_enabled()){
			return 1;
		}
        s = select(camera->dev_fd+1, &fds, NULL, NULL, &t);
        //读取缓存
        if (s > 0) {
            if (ioctl(camera->dev_fd, VIDIOC_DQBUF, &camera->buf) < 0) {
                fprintf(stderr, "ioctl error:get_frame:%s\n", strerror(errno));
                return 0;
            }
            break;
        }
        FD_ZERO(&fds);
        FD_SET(camera->dev_fd, &fds);
        t.tv_sec = 1;
        t.tv_usec = 0;
    }
    switch (camera->format)
    {
       case V4L2_PIX_FMT_MJPEG:
       /*    if (camera->buf.bytesused <= HEADFRAME1)
           {
               msg_out("ignore empty frame.../n");
               return 0;
           }
           // we can save tmp_buff to a jpg file,just write it!
           memcpy(vd_info->tmp_buffer, vd_info->mem[vd_info->buf.index],
                  vd_info->buf.bytesused);
           // here decode MJPEG,so we can dispaly it
           if (jpeg_decode(&vd_info->frame_buffer, vd_info->tmp_buffer,
                           &vd_info->width, &vd_info->height) < 0 )
           {
               msg_out("decode jpeg error/n");
               goto err;
           }
           break;
           */
       case V4L2_PIX_FMT_YUYV:
    	   yuyv_2_rgb888(camera->mem_list[camera->buf.index].addr, camera->tempbuff.temp, camera->info.width, camera->info.height);
    	   break;
       default:
    	   printf("Invalid video format setting\n");
    	   return 0;
           break;
    }

    /*设备重新放入缓存队列*/
    if (ioctl(camera->dev_fd, VIDIOC_QBUF, &camera->buf) < 0) {
        fprintf(stderr, "ioctl error:get_frame QBUF:%s\n", strerror(errno));
        return 0;
    }
    return 1;
}


