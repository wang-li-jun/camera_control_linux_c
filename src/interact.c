#include "interact.h"
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
#include "device.h"

static pthread_t _accept;
static int s_socket; //interact socket

static void limit_to_local_access(int fd)
{

}

static void command_dealer(int csockfd, char* buffer)
{
	printf("get command:%s\n",buffer);
	char* response;
	if(strcmp(buffer,"cameraCommandTest") == 0){
		response = "Test Echo\n";
	}else if(strcmp(buffer,"cameraOpen") == 0){
		if(is_dev_enabled()){
			response = "Already Opened\n";
		}else{
			if(dev_enable()){
				response = "Open camera successful\n";
			}else{
				response = "Open camera failed\n";
			}
		}
	}else if(strcmp(buffer,"cameraClose") == 0){
		if(is_dev_enabled()){
			if(dev_disable()){
				response = "Close camera successful\n";
			}else{
				response = "Close camera failed\n";
			}
		}else{
			response = "Already Closed\n";
		}
	}else{
		response = "Invalid command\n";
	}
	printf("response:%s",response);
	write(csockfd, response, strlen(response));
}

static void *pth_accept(void* arg)
{
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int csockfd = -1;
    ssize_t size = -1;
    char buffer[1024];
    while (1) {
    	memset(buffer,0,sizeof(buffer)/sizeof(char));
    	csockfd = accept(s_socket, (struct sockaddr*)&caddr, &len);
        if (csockfd < 0) {
            fprintf(stderr, "interact pth_accept:accept:%s\n", strerror(errno));
            continue;
        } else {
        	limit_to_local_access(csockfd);
        	size = read(csockfd, buffer, sizeof(buffer));
        	command_dealer(csockfd,buffer);
        	close(csockfd);
        }
    }
    return (void*)1;
}

int start_interact_server()
{
	//construct a socket
	if(( s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	   fprintf(stderr, "start_interact error:socket:%s\n", strerror(errno));
	   return s_socket;
	}

	// Enable address reuse
	int on = 1;
	if( setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ){
	   fprintf(stderr, "start_interact error:socket option:%s\n", strerror(errno));
	   return s_socket;
	}

	//construct an address
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(INTERACT_PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;
	memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));

	/*bind the socket and saddress*/
	socklen_t len=sizeof(saddr);
	if (bind(s_socket, (struct sockaddr*)&saddr, len) < 0) {
	   fprintf(stderr, "start_interact error:bind:%s\n", strerror(errno));
	   return -1;
	}

	/*notify the system to be ready to accept*/
	if (listen(s_socket, 1) < 0) {
	   fprintf(stderr, "start_interact error:listen:%s\n", strerror(errno));
	}

	//start accept thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param param;
	param.sched_priority = 80;
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&_accept, &attr, pth_accept, (void*)0) != 0) {
		fprintf(stderr, "start_interact error:create accept thread:%s\n", strerror(errno));
		return -1;
	}
	pthread_attr_destroy(&attr);

	return s_socket;
}

void stop_interact_server()
{
    if (s_socket > 2) {
        close(s_socket);
    }
}
