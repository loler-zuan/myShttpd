#include "shttpd.h"

//可以使用宏定义 -D来控制是否打印标记信息。标记信息一般用于debug阶段，print想要的结构

//初始化phtread_mutex_t类型的变量时有两种方法：一种为使用宏PTHREAD_MUTEX_INITIALIZER进行初始化，另外一种是使用函数pthread_mutex_init函数。第一种方法仅局限于静态初始化的时候使用：将“声明”、“定义”、“初始化”一气呵成，除此之外的情况都只能使用pthread_mutex_init函数。
extern struct conf_options conf;
static struct worker *workers=NULL;
static int count=0;
pthread_mutex_t protectWorkers=PTHREAD_MUTEX_INITIALIZER;
void Worker_Init();
void Worker_Add(int i);
void Worker_Delete(int i);
void Worker_Destory();
void do_worker(struct worker_area *workerarea)
{
	int sock=workerarea->away;
	fd_set fds;
	struct timeval timeout;
	int res=1;
	int nbytes=0;
	printf("%d:",workerarea->away);
	while(res>0)
	{
		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		timeout.tv_sec=conf.keepAlive;
		timeout.tv_usec=0;
		res=select(FD_SETSIZE,&fds,NULL,NULL,&timeout);//这里最大值设置可以优化
		printf("here wrong:%d\n",res);
		switch(res)
		{
			case -1:
			case 0:close(sock);return;
			default:if(FD_ISSET(sock,&fds))
					{
						memset(workerarea->req,0,sizeof(workerarea->req));
						nbytes=read(sock,workerarea->req,sizeof(workerarea->req));
						if(nbytes==0)
						{
							//链接已关闭
							return;
						}
						workerarea->req_len=nbytes;
						Request_Parse(workerarea);
						Request_Response(workerarea);
						printf("recv bytes:%d\n",nbytes);
						printf("%s",workerarea->req);
					}
		}
	}
	printf("analysis data....\n");
}
void *worker(void *arg)
{
	struct worker *worker=(struct worker*)arg;
	struct worker_ctl *ctl=&(worker->ctl);
	pthread_mutex_unlock(&protectWorkers);
	for(;ctl->flags!=WORKER_DETACHING;)
	{
		int res=pthread_mutex_trylock(&(ctl->mutex));
		if(res!=0)
		{
			//之前已经阻塞
			sleep(1);
			continue;
		}
		else
		{
			printf("WORKER_RUNNING\n");
			ctl->flags=WORKER_RUNNING;
			do_worker(&(worker->area));		
			close(worker->area.away);
			if(ctl->flags==WORKER_DETACHING)
				break;
			else
			{
				ctl->flags=WORKER_IDEL;
			}
		}
	}
	ctl->flags=WORKER_DETACHED;
	count--;
}
void Worker_Add(int i)
{
	int res=-1;
	int err=0;
	if(workers[i].ctl.flags==WORKER_RUNNING)
		return;
	else
	{
		pthread_mutex_lock(&protectWorkers);//保证在worker[i]传递给线程之前只被这个线程修改。
		workers[i].ctl.flags=WORKER_IDEL;
		err=pthread_create(&(workers[i].ctl.pid),NULL,worker,(void *)&workers[i]);
		count++;
		return;
	}
}
void Worker_Init()
{
	int i=0;
	workers=(struct worker*)malloc(sizeof(struct worker)*conf.maxClient);
	memset(workers,0,sizeof(struct worker)*conf.maxClient);
	for(i=0;i<conf.maxClient;i++)	
	{
		pthread_mutex_init(&workers[i].ctl.mutex,NULL);
		pthread_mutex_lock(&workers[i].ctl.mutex);
		workers[i].ctl.flags=WORKER_INITED;//当前工作者状态
		workers[i].area.away=-1;
		workers[i].area.local=-1;
		memset(workers[i].area.req,0,sizeof(workers[i].area.req));
		memset(workers[i].area.res,0,sizeof(workers[i].area.res));
		//这里的初始化未完成，因为结构体没有完成，先完成线程调度部分
	}
	for(i=0;i<conf.initClient;i++)
	{
		Worker_Add(i);
	}
}

int findStatus(int status)
{
	int i=0;
	for(i=0;i<count;i++)
	{
		if(workers[i].ctl.flags == status)
			return i;
	}
	return -1;
}

void Worker_ScheduleRun(int sock)
{
	struct sockaddr_in client;
	int len=sizeof(client);
	int res=-1;
	static int wait=0;
	int clientSock;
	Worker_Init();

	struct timeval tv;
	fd_set fds;
	while(1)
	{
		FD_ZERO(&fds);//每次循环都要清空集合，否则检测不到描述符变化
		FD_SET(sock,&fds);

		tv.tv_sec=0;
		tv.tv_usec=500000;//轮询时间0.5秒
		res=select(FD_SETSIZE,&fds,NULL,NULL,&tv);//这里最大值设置可以优化
		switch(res)
		{
			case -1:
			case 0:continue;break;
			default:
					if(FD_ISSET(sock,&fds))
					{
						if(wait==0)
						{
							clientSock=accept(sock,(struct sockaddr *)&client,&len);//一次accept处理一次数据，但是如果一个sock同时有多个请求，缓冲区就还有没有处理的请求数据。select会返回>0
							printf("client coming\n");
						}
						//查找空闲线程，如果没有新建，保证有空闲线程后，赋值clientSock，并解锁。
						int idel=findStatus(WORKER_IDEL);
						if(idel==-1)
						{
							//printf("cant find idel thread\n");
							if(count<conf.maxClient)
							{
							//	printf("add new thread\n");
								Worker_Add(count);
								idel=count;
							}
							else
							{
								wait=1;
								continue;
							}
						}
						if(idel!=-1)
						{
							//printf("will work\n");
							workers[idel].area.away=clientSock;
							pthread_mutex_unlock(&(workers[idel].ctl.mutex));
							wait=0;
						}
					}
		}
	}
	return;
}
