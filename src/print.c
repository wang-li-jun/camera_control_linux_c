#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>
#include <time.h>
#include <limits.h>
#include <jpeglib.h>

extern void yuyv_2_rgb888(char *src, char *target, int width, int height)
{
    int           i,j;
    unsigned char y1,y2,u,v;
    int r1,g1,b1,r2,g2,b2;
    char * pointer;

    pointer = src;

    for(i = 0;i < height;i++)
    {
		for(j = 0;j < width/2;j++)// Every round 4 bytes => 2 pixel, after transformed to RGB, then it becomes 6 bytes.
		{
			y1 = *( pointer + (i*width/2+j)*4);
			u  = *( pointer + (i*width/2+j)*4 + 1);
			y2 = *( pointer + (i*width/2+j)*4 + 2);
			v  = *( pointer + (i*width/2+j)*4 + 3);

			r1 = y1 + 1.042*(v-128);
			g1 = y1 - 0.34414*(u-128) - 0.71414*(v-128);
			b1 = y1 + 1.772*(u-128);

			r2 = y2 + 1.042*(v-128);
			g2 = y2 - 0.34414*(u-128) - 0.71414*(v-128);
			b2 = y2 + 1.772*(u-128);

			if(r1>255)
			r1 = 255;
			else if(r1<0)
			r1 = 0;

			if(b1>255)
			b1 = 255;
			else if(b1<0)
			b1 = 0;

			if(g1>255)
			g1 = 255;
			else if(g1<0)
			g1 = 0;

			if(r2>255)
			r2 = 255;
			else if(r2<0)
			r2 = 0;

			if(b2>255)
			b2 = 255;
			else if(b2<0)
			b2 = 0;

			if(g2>255)
			g2 = 255;
			else if(g2<0)
			g2 = 0;

			*(target + (i*width/2+j)*6    ) = (unsigned char)r1;
			*(target + (i*width/2+j)*6 + 1) = (unsigned char)g1;
			*(target + (i*width/2+j)*6 + 2) = (unsigned char)b1;
			*(target + (i*width/2+j)*6 + 3) = (unsigned char)r2;
			*(target + (i*width/2+j)*6 + 4) = (unsigned char)g2;
			*(target + (i*width/2+j)*6 + 5) = (unsigned char)b2;
		}
    }
}

extern int encode_jpeg_to_file(char *inbuf,int width,int height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int row_stride;
    unsigned char *buf=NULL;
    int x;

    FILE *fptr_jpg = fopen("./image_jpeg.jpg","wb");
    if(fptr_jpg == NULL)
    {
    	printf("Encoder:open file failed!/n");
    	return 0;
    }
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fptr_jpg);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80,TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * 3;
    buf = malloc(row_stride);
    row_pointer[0] = buf;
    while (cinfo.next_scanline < height)
    {
    	for (x = 0; x < row_stride; x+=3)
    	{
			buf[x]   = inbuf[x];
			buf[x+1] = inbuf[x+1];
			buf[x+2] = inbuf[x+2];
    	}
    	jpeg_write_scanlines (&cinfo, row_pointer, 1);//critical
    	inbuf += row_stride;
    }

    jpeg_finish_compress(&cinfo);
    fclose(fptr_jpg);
    jpeg_destroy_compress(&cinfo);
    free(buf);
    return 1;
}

extern int encode_jpeg(char *inbuf,unsigned char **outbuf,unsigned long *outsize,int width,int height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo,outbuf,outsize);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80,TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * 3;
    while(cinfo.next_scanline < height)
    {
    	row_pointer[0] = & inbuf[cinfo.next_scanline * row_stride];
    	jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return 1;
}
