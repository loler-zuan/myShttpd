#include "shttpd.h"

struct conf_options conf={
	"/home/www/cgi",
	"index.html",
	"/home/www",
	"/home/conf/shttpd.conf",
	80,
	4,
	3 ,
	15,
	2
};
void printOptions()
{
	printf("the options:\n");
	printf("cgiRoot:%s\n",conf.cgiRoot);
	printf("defaultFile:%s\n",conf.defaultFile);
	printf("documentRoot:%s\n",conf.documentRoot);
	printf("configFile:%s\n",conf.configFile);
	printf("port:%d\n",conf.port);
	printf("maxClient:%d\n",conf.maxClient);
	printf("timeOut:%d\n",conf.timeOut);
	printf("keepAlive:%d\n",conf.keepAlive);
	printf("initClient:%d\n",conf.initClient);
	printf("\n\n\n");
}
int do_listen()
{
	int sock,client;
	struct sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_port=htons(conf.port);
	server.sin_addr.s_addr=htonl(INADDR_ANY);

	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock==-1)
	{
		printf("can't create socket\n");
		return -1;
	}
	bind(sock,(struct sockaddr *)&server,sizeof(server));
	listen(sock,conf.maxClient*2);
	return sock;
}
int main(int argc,char *argv[])
{
	int s=-1;
	Para_Init(argc,argv);
	printOptions();
	s=do_listen();
	if(s==-1)
	{
		exit(0);
	}
	Worker_ScheduleRun(s);
	return 0;
}
