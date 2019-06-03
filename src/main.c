#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "main.h"
#include "tcp_server.h"
#include "interact.h"
#include "print.h"
#include "device.h"

/*clean up all*/
static void close_all()
{
	destroy_camera(&devconf);
	close_ssockfd();
	stop_interact_server();
}

/*信号处理函数*/
static void sig_handler(int signo)
{
    if (signo == SIGINT) {
        close_all();
        exit(1);
    }
}

int main(int argc, char* argv[])
{
	//get_camera_info();
	//return 0;

    if (argc < 2) {
        fprintf(stderr, "-usage:%s <serverport>\n", argv[0]);
        exit(0);
    }

	char *video_file = "/dev/video0";
	char *server_port = argv[1];

    /*设置信号处理函数*/
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        fprintf(stderr, "signal error:main:%s\n", strerror(errno));
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal error:main:%s\n", strerror(errno));
    }

    //给dev_config设置配置信息
    set_dev_config();

    /*初始化设备*/
    if (!init_camera(video_file, &devconf)){
        raise(SIGINT);
    }

    /*建立网络服务*/
    if (init_server(atoi(server_port), 10) < 0) {
        raise(SIGINT);
    }

    /*make local connection with PHP method*/
    start_interact_server();

    enable(devconf.dev_fd);//让设备开始摄像
    printf("Enable camera device successful\n");

    while (1) {
    	if(is_dev_enabled()){
    		if (!get_frame(&devconf)) {//主线程采集视屏信息
				printf("Unable to get frame from the camera\n");
				break;
			}
    	}else{
    		sleep(1);
    	}
	}
    close_all();
	return 0;
}
