#include "tcp_server.h"
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>

static pthread_t _accept;
static int ssockfd; //camera web server socket fd

/*发送数据给客户端*/
static void *pth_send_picture(void* arg)
{
    int sockfd = *(int*)arg;
    char buffer[1024];
    ssize_t size = -1;
   	size = read(sockfd, buffer, sizeof(buffer));
    if (size < 0 )
    {
    	fprintf(stderr, "read :%s\n", strerror(errno));
    }
    else if (size > 0)
    {
    	  //printf("read buffer %s\n",buffer);
          write(sockfd, http_header, strlen(http_header));
          if (strstr(buffer, "snap") != NULL) {
				printf("pth_send_picture snap\n");
				//printf("temp %d %d\n",devconf.tempbuff.temp,devconf.tempbuff.length);
				unsigned char* pBuf = NULL;
				unsigned long iLen = 0;
				encode_jpeg(devconf.tempbuff.temp,&pBuf,&iLen,devconf.info.width, devconf.info.height);
				memset(buffer, 0, 1024);
				sprintf(buffer,snap_header,iLen);
				write(sockfd, buffer, strlen(buffer));
				write(sockfd, pBuf, iLen);
				free(pBuf);
				pBuf = NULL;
				close(sockfd);
          } else if (strstr(buffer, "stream") != NULL) {
        	  printf("pth_send_picture stream\n");
              int ret = 0;
              unsigned char* pBuf = NULL;
              unsigned long iLen = 0;
              write(sockfd, stream_header, strlen(stream_header));
              while (1) {
                 ret = write(sockfd, boundary, strlen(boundary));
                 if (ret < 0) {//客户端关掉时线程结束
                	 printf("connection closed\n");
                	 close(sockfd);
                     break;
                 }
				 encode_jpeg(devconf.tempbuff.temp,&pBuf,&iLen,devconf.info.width, devconf.info.height);
				 memset(buffer, 0, 1024);
				 sprintf(buffer, snap_header, iLen);
				 write(sockfd, buffer, strlen(buffer));
				 write(sockfd, pBuf, iLen);
				 free(pBuf);
				 pBuf = NULL;
                 usleep(1000000 / devconf.fps);
              }
         } else {
        	 close(sockfd);
         }
    }
    return (void*)1;
}

/*接收连接线程*/
static void *pth_accept(void* arg)
{
    pthread_t pth;
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int csockfd = -1;
    int err;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    while (1) {
        csockfd = accept_client(ssockfd, (struct sockaddr*)&caddr, &len);
        printf("csockfd:%d\n", csockfd);
        show_client_ip(csockfd);
        if (csockfd < 0) {
            fprintf(stderr, "pth_accept:accept:%s\n", strerror(errno));
            continue;
        } else {
        	err = pthread_create(&pth, &attr, pth_send_picture, (void*)&csockfd);
        	if (err != 0) {
        		fprintf(stderr, "pthread_create:%s\n", strerror(errno));
        	}
        }
    }
    return (void*)1;
}

int init_server(unsigned int port, int backlog)
{
   //construct a socket
   if(( ssockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       fprintf(stderr, "init_server error:socket:%s\n", strerror(errno));
       return ssockfd;
   }

   // Enable address reuse
   int on = 1;
   if( setsockopt(ssockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ){
	   fprintf(stderr, "init_server error:socket option:%s\n", strerror(errno));
	   return ssockfd;
   }

   //construct an address
   struct sockaddr_in saddr;
   saddr.sin_family = AF_INET;
   saddr.sin_port = htons(port);
   saddr.sin_addr.s_addr = INADDR_ANY;
   memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));

   /*bind the socket and saddress*/
   socklen_t len=sizeof(saddr);
   if (bind(ssockfd, (struct sockaddr*)&saddr, len) < 0) {
       fprintf(stderr, "init_server error:bind:%s\n", strerror(errno));
       return -1;
   }

    /*notify the system to be ready to accept*/
   if (listen(ssockfd, backlog) < 0) {
       fprintf(stderr, "init_server error:listen:%s\n", strerror(errno));
   }

   /*建立与客户端的连接*/
   int err;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   struct sched_param param;
   param.sched_priority = 80;
   pthread_attr_setschedpolicy(&attr, SCHED_RR);
   pthread_attr_setschedparam(&attr, &param);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   err = pthread_create(&_accept, &attr, pth_accept, (void*)0);
   if (err != 0) {
       fprintf(stderr, "pthread_create:%s\n", strerror(errno));
       return -1;
   }
   pthread_attr_destroy(&attr);

   return ssockfd;
}

static int set_flag(int ssockfd)
{
    int oldflag;
    oldflag = fcntl(ssockfd, F_GETFL);
    oldflag &= (~O_NONBLOCK);
    //fcntl(ssockfd, F_SETFL, oldflag|O_NONBLOCK);
    fcntl(ssockfd, F_SETFL, oldflag);
    return ssockfd;
}

int accept_client(int ssockfd, struct sockaddr *restrict_addr, socklen_t *restrict_len)
{
    return set_flag(accept(ssockfd, restrict_addr, restrict_len));
}

extern void show_client_ip(int fd)
{
	struct sockaddr_in peerAddr;
	socklen_t peerLen = sizeof(peerAddr);
	char ipAddr[INET_ADDRSTRLEN];
	if (getpeername(fd, (struct sockaddr *)&peerAddr, &peerLen) != 0) {
		fprintf(stderr, "get peer name failed:%s\n", strerror(errno));
	} else {
		printf("connected peer address = %s:%d\n", inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr)), ntohs(peerAddr.sin_port));
	}
}

void close_ssockfd()
{
    if (ssockfd > 2) {
        close(ssockfd);
    }
}

