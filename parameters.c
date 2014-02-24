#include "shttpd.h"
extern struct conf_options conf;
static void help(void)
{
	printf("sHTTPD -p port  -m number  -i number -d path  -c  path   -f  filename   -t seconds  -g filename -a seconds -h -v\n");
	printf("sHTTPD --port number\n");
	printf("       --initClient number\n");
	printf("       --maxClient number\n");
	printf("       --documentRoot) path\n");
	printf("       --defaultFile) filename\n");
	printf("       --cgiRoot path \n");
	printf("       --defaultFile filename\n");
	printf("       --timeOut seconds\n");
	printf("       --keepAlive seconds\n");
	printf("       --configFile filename\n");
	exit(0);
}

static void display_para()
{
	printf("sHTTPD ListenPort: %d\n",conf.port);  
	printf("       MaxClient: %d\n", conf.maxClient);
	printf("       DocumentRoot: %s\n",conf.documentRoot);
	printf("       DefaultFile:%s\n",conf.defaultFile);
	printf("       CGIRoot:%s \n",conf.cgiRoot);
	printf("       DefaultFile:%s\n",conf.defaultFile);
	printf("       TimeOut:%d\n",conf.timeOut);
	printf("       keepAlive:%d\n",conf.keepAlive);
	printf("       ConfigFile:%s\n",conf.configFile);
}
void version()
{
	printf("easyShttpd 1.0 by Code\n");
	exit(0);
}
void Cmd_Analyse(int argc,char *argv[])
{
	char *ops="c:f:d:g:p:m:t:i:a:hv";
	int c;
	struct option options[]=
	{
		{"cgiRoot",1,NULL,'c'},
		{"defaultFile",1,NULL,'f'},
		{"documentRoot",1,NULL,'d'},
		{"configFile",1,NULL,'g'},
		{"port",1,NULL,'p'},
		{"maxClient",1,NULL,'m'},
		{"timeOut",1,NULL,'t'},
		{"initClient",1,NULL,'i'},
		{"help",0,NULL,'h'},
		{"version",0,NULL,'v'},
		{"keepAlive",1,NULL,'a'},
		{0,0,0,0}
	};
	while((c=getopt_long(argc,argv,ops,options,NULL))!=-1)
	{
		switch(c)
		{
			case 'c':strcpy(conf.cgiRoot,optarg);break;
			case 'f':strcpy(conf.defaultFile,optarg);break;
			case 'd':strcpy(conf.documentRoot,optarg);break;
			case 'g':strcpy(conf.configFile,optarg);break;//optarg全是字符串
			case 'p':conf.port=strtol(optarg,NULL,10);break;
			case 'm':conf.maxClient=strtol(optarg,NULL,10);break;
			case 'i':conf.initClient=strtol(optarg,NULL,10);break;
			case 't':conf.timeOut=strtol(optarg,NULL,10);break;
			case 'a':conf.keepAlive=strtol(optarg,NULL,10);break;
			case 'h':help();break;
			case 'v':version();break;
//			case '?':printf("wrong para\n");exit(0);break;
			default :help();break;
		}
	};
	return;
}

int readline(int fd,char *buf,int len)
{
	int i;
	int n=-1;
	for(i=0;i<len;i++)
	{
		n=read(fd,buf+i,1);
		//printf("%c %d\n",*(buf+i),n);
		if(n==0)
		{
			*(buf+i)='\0';
			break;
		}
		else if(*(buf+i) == '\r' || *(buf+i) == '\n')
		{
			*(buf+i)='\n';
			break;
		}
	}
	return i;
}
void File_Analyse()
{
	char bytes[256];
	char *p=NULL;
	char *name,*value;
	int fd=-1;
	fd = open(conf.configFile,O_RDONLY);
	if(fd == -1)
	{
		printf("cant open the configFile\n");
		return;
	}
	memset(bytes,0,sizeof(bytes));
	while(readline(fd,bytes,sizeof(bytes))>0)
	{
		p=bytes;
		//printf("%s\n",bytes);
		JUMPOVER_CHAR(p,32);
		if(*p=='#')
		{
			continue;
		}
		else
		{
			name=p;
			while(*p!='='&& *p!=' ')
			{
				p++;
			}
			*p=0;
			p++;
	//		printf("%c\n",*p);
			JUMPOVER_CHAR(p,32);
	//		printf("%c\n",*p);
			JUMPOVER_CHAR(p,'=');
	//		printf("%c\n",*p);
			JUMPOVER_CHAR(p,32);
	//		printf("%c\n",*p);
			value=p;
			while(*p!=' ')
			{
				p++;
			}
			*p=0;
			//printf("name:%s,value:%s\n",name,value);
			if(strncmp("cgiRoot",name,7)==0)
				memcpy(conf.cgiRoot,value,strlen(value)+1);
			else if(strncmp("defaultFile",name,11)==0)
				memcpy(conf.defaultFile,value,strlen(value)+1);
			else if(strncmp("documentRoot",name,12)==0)
				memcpy(conf.documentRoot,value,strlen(value)+1);
			else if(strncmp("port",name,4)==0)
				conf.port=strtol(value,NULL,10);
			else if(strncmp("maxClient",name,9)==0)
				conf.maxClient=strtol(value,NULL,10);
			else if(strncmp("timeOut",name,7)==0)
				conf.timeOut=strtol(value,NULL,10);
			else if(strncmp("keepAlive",name,9)==0)
				conf.keepAlive=strtol(value,NULL,10);
		}
		memset(bytes,0,sizeof(bytes));
	}
	//printf("port:%d\n",conf.port);
	close(fd);
}

void Para_Init(int argc,char *argv[])
{
	Cmd_Analyse(argc,argv);
	File_Analyse();
}
