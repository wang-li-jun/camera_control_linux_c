#ifndef __PRINT_H__
#define __PRINT_H__

extern int print_picture(int fd, unsigned char *buf, int size);
extern void yuyv_2_rgb888(char *src, char *target, int width, int height);
extern int encode_jpeg_to_file(char *inbuf,int width,int height);
extern int encode_jpeg(char *inbuf,unsigned char **outbuf,unsigned long *outsize,int width,int height);


#endif
