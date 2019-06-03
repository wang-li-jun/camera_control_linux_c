#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <sys/socket.h>
#include "device.h"

/*http protocol start*/
static char http_header[] = "HTTP/1.1 200 OK\r\n"
                     "server:Apache/2.4.3 (Unix)\r\n"
                     "cache-control:no-cache, must-revalidate, pre-check=0"
                     "max-age=0\r\n"
                     "pragma: no-cache\r\n";
static char snap_header[] = "content-type:image/jpeg\n"
                             "content-length: %d\n\n";
static char boundary[] = "--www.nokia.com\n";
static char stream_header[] = "content-type: multipart/x-mixed-replace;"
                        "boundary=www.nokia.com\r\n\r\n";
/*http protocal end*/


extern int init_server(unsigned int port, int backlog);
extern int accept_client(int ssockfd, struct sockaddr *restrict_addr, socklen_t *restrict_len);
extern void show_client_ip(int fd);
extern void close_ssockfd();


#endif
