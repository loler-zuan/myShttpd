#include <pthread.h>
#include <stddef.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#define  JUMPOVER_CHAR(p,over) for(;*p==over;p++);
#define JUMPTO_CHAR(p,to) for(;*p!=to;p++);
#define HDR_DATE 1
#define HDR_INT 2
#define HDR_STRING 3
#define OFFSET(x)   offsetof(struct headers, x)
struct string
{
	    char *s;
		int length;
};
struct headers{
	long cl;           /* Content-Length:      */
	struct string ct;          /* Content-Type:        */
	struct string useragent;   /* User-Agent:          */
	time_t ims;        /* If-Modified-Since:       */
	struct string auth;        /* Authorization        */
	struct string referer;     /* Referer:         */
	struct string cookie;      /* Cookie:          */
	struct string location;    /* Location:            */
	long status;       /* Status:          */
	struct string range;       /* Range:           */
	struct string connection;  /* Connection:           */
	struct string transenc;    /* Transfer-Encoding:       */
};
struct http_header {
	int     len;        /* Header name length       */
	int     type;       /* Header type          */
	size_t      offset;     /* Value placeholder        */
	char    *name;      /* Header name          */
};
struct conf_options
{
	char cgiRoot[128];
	char defaultFile[128];
	char documentRoot[128];
	char configFile[128];
	int port;
	int maxClient;
	int timeOut;//超时时间 能够访问服务器，打开web页面，但是页面图片太多，还没等图片下载完，就已经完成了，就是超时时间问题。
	int keepAlive;//保持连接时间
	int initClient;
};

enum STATUS{ WORKER_INITED, WORKER_RUNNING,WORKER_DETACHING, WORKER_DETACHED,WORKER_IDEL };
//被初始化，正在运行，被请求中断，已经中断，空闲。
enum METHODS{GET,HEAD,POST,PUT,TRACE,CONNECT,OPTION};
struct req_line
{
	char  *method;
	char *url;
	char *version;
};
struct worker_recv
{
	struct req_line reqLine;
	struct headers header;
	char *body;
};
/*struct status_line
{
	char *version;//这里为指针，首先应该在res区域给他一块区域，否则空指针错误
	char *statusID;
	char *statusPhrase;

};*/
struct worker_send
{
	//struct status_line statusLine;
	//struct headers header;
	//char *body;
};
struct worker_ctl//任务控制
{
	pthread_t pid;//任务分配的线程id
	pthread_mutex_t mutex;//控制线程执行任务的锁
	int flags;
};

struct worker_area//每个任务的工作区间
{
	int local;//读取本地文件的
	int away;//要发送的文件描述符
	struct worker_recv recv;
	struct worker_send send;
	char req[64*1024];
	long req_len;
	char res[64*1024];	
	long res_len;
	int err;
	int file_length;
};

struct worker
{
	struct worker_ctl ctl;
	struct worker_area area;
};
struct mime_type{
	char    *extension;
	int             type;
	int         ext_len;
	char    *mime_type;
};

//struct mine_type* Mine_Type(char *uri, int len);
