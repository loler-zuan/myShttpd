#include "shttpd.h"
extern struct conf_options conf;
static struct http_header http_headers[] = {
	{16,    HDR_INT,    OFFSET(cl),         "Content-Length: "      },
	{14,    HDR_STRING, OFFSET(ct),         "Content-Type: "        },
	{12,    HDR_STRING, OFFSET(useragent),  "User-Agent: "      },
	{19,    HDR_DATE,   OFFSET(ims),            "If-Modified-Since: "   },
	{15,    HDR_STRING, OFFSET(auth),       "Authorization: "       },
	{9, HDR_STRING, OFFSET(referer),        "Referer: "         },
	{8, HDR_STRING, OFFSET(cookie),     "Cookie: "          },
	{10,    HDR_STRING, OFFSET(location),       "Location: "            },
	{8, HDR_INT,    OFFSET(status),     "Status: "              },
	{7, HDR_STRING, OFFSET(range),      "Range: "               },
	{12,    HDR_STRING, OFFSET(connection), "Connection: "          },
	{19,    HDR_STRING, OFFSET(transenc),   "Transfer-Encoding: "   },
	{0, HDR_INT,    0,                  NULL                }
};
int ParseReqLine(struct worker_area *workerarea)
{
	char path[1024];
    char *q;
	char *url;
	printf("parse request line ......");
	char *p=workerarea->req;
	struct worker_recv *recv=&(workerarea->recv);
	JUMPOVER_CHAR(p,' ');
	memset(path,0,sizeof(path));
	recv->reqLine.method=p;
	JUMPTO_CHAR(p,' ');
	*p='\0';   //method

	p++;
	JUMPOVER_CHAR(p,' ');
	recv->reqLine.url=p;
	JUMPTO_CHAR(p,' ');
	*p='\0';//url
		
	url=workerarea->recv.reqLine.url;
	
	/*确定打开文件的路径*/
	memset(path,sizeof(path),0);
	memcpy(path,conf.documentRoot,strlen(conf.documentRoot));
	memcpy(path+strlen(conf.documentRoot),url,strlen(url)+1);
	q=path;
	JUMPTO_CHAR(q,'\0');
	q--;
	printf("path1:%s\n",path);
	if(*q=='/')
	{
		q++;
		memcpy(q,conf.defaultFile,strlen(conf.defaultFile)+1);	
	}
	printf("path2:%s\n",path);
	workerarea->local=open(path,O_RDONLY);
	if(workerarea->local==-1)
	{
		workerarea->err=404;
	}
	else
	{
		workerarea->err=200;
	}	
	p++;
	printf("err:%d\n",workerarea->err);
	JUMPOVER_CHAR(p,' ');
	recv->reqLine.version=p;
	printf("jumpover is right\n");
	JUMPTO_CHAR(p,'\r');
	*p='\0';
	printf("over\n");
	return 0;
}
static int
montoi(char *s)
{
	int retval = -1;
	static char *ar[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	size_t  i;

	for (i = 0; i < sizeof(ar) / sizeof(ar[0]); i++)
		if (!strcmp(s, ar[i])){
			retval = i;
			return retval;
		}
	return retval;
}

static time_t
date_to_epoch(char *s)
{
	printf("==>date_to_epoch\n");
	struct tm   tm;
	char        mon[32];
	int     sec, min, hour, mday, month, year;
	memset(&tm, 0, sizeof(tm));
	sec = min = hour = mday = month = year = 0;
	if (((sscanf(s, "%d/%3s/%d %d:%d:%d",
		 &mday, mon, &year, &hour, &min, &sec) == 6) ||
		 (sscanf(s, "%d %3s %d %d:%d:%d",
		 &mday, mon, &year, &hour, &min, &sec) == 6) ||
		 (sscanf(s, "%*3s, %d %3s %d %d:%d:%d",
		 &mday, mon, &year, &hour, &min, &sec) == 6) ||
		 (sscanf(s, "%d-%3s-%d %d:%d:%d",
		 &mday, mon, &year, &hour, &min, &sec) == 6)) &&
		 (month = montoi(mon)) != -1) {
		 tm.tm_mday  = mday;
		 tm.tm_mon   = month;
		 tm.tm_year  = year;
		 tm.tm_hour  = hour;
		 tm.tm_min   = min;
		 tm.tm_sec   = sec;
		}
		if (tm.tm_year > 1900)
			tm.tm_year -= 1900;
		else if (tm.tm_year < 70)
			tm.tm_year += 100;
		printf("<==date_to_epoch\n");
		return (mktime(&tm));
}

int ParseHead(struct worker_area *workerarea)
{
	printf("parse head .....");
	char *s=workerarea->req;
	int length=workerarea->req_len;
	char *e=s+length;
	char *p=s;//p为行尾
	struct headers *header= &(workerarea->recv.header);
	struct http_header  *h;
	int find=-1;
	while(1)
	{
		JUMPTO_CHAR(s,'\r');
		if(*(++s)=='\n')
		{
			length -= s-workerarea->req;
		}
			break;
	}//跳过请求行
	s++;
	//printf("%.*s\n",length,s);
	while(s < e)
	{
		 while(p < e)
		{
			JUMPTO_CHAR(p,'\r');
			if(*(++p)=='\n')
				break;
		}

//		printf("s:%s\n",s);
		for(h=http_headers;h->len != 0;h++)
		{
//		printf("s:%.*s   h->name:%s\n",h->len,s,h->name);
			//printf("    diff:%d\n",strncmp(h->name,s,h->len));
			if(strncmp(h->name,s,h->len)==0)//存在
			{
//					printf("%s\n",h->name);
				find=1;
				s+=h->len;
				break;
			}
		}
		if(find==1)
		{
			switch (h->type)
			{
				case HDR_STRING://跳过首部名称部分
							(*(struct string *)((char *)header+h->offset)).s=s;
							(*(struct string *)((char *)header+h->offset)).length=p-s-1;
							break;
				case HDR_INT:
							(*(long *)((char *)header+h->offset))=strtoul(s, NULL, 10);
							 break;
				case HDR_DATE:
							(*(time_t *)((char *)header+h->offset))=date_to_epoch(s);
							break;
			}
		}
		find=0;
		if(*(p+1)=='\r' && *(p+2)=='\n')
			break;
		s=p+1;
	}
	printf("over\n");
	return;
}
int ParseBody(struct worker_area *workerarea)
{
	printf("parse body part ......");
	char *p=workerarea->req;
	while(1)
	{
		JUMPTO_CHAR(p,'\r');
		if(*(++p)=='\n'&& *(++p)=='\r' && *(++p)=='\n' )
			break;
	}//跳过首部
	p++;
	workerarea->recv.body=p;
	printf("over\n");
}
void printfHeaders(struct headers *header)
{
	printf("Content-length: %ld\n",header->cl);
	printf("Content-Type: %.*s\n",header->ct.length,header->ct.s);
	printf("User-Agent : %.*s\n",header->useragent.length,header->useragent.s);
//	printf("If-Modified-Since: %.*s\n",header->ct.length,header->ct.s);
	printf("Referer: %.*s\n",header->referer.length,header->referer.s);
	printf("Cookie: %.*s\n",header->cookie.length,header->cookie.s);
	printf("Location: %.*s\n",header->location.length,header->location.s);
	printf("Authorization: %.*s\n",header->auth.length,header->auth.s);
	printf("Range: %.*s\n",header->location.length,header->location.s);
	printf("Connection: %.*s\n",header->connection.length,header->connection.s);
	printf("Transfer-Encoding: %.*s\n",header->transenc.length,header->transenc.s);
	printf("Status: %ld\n",header->status);
}
int Request_Parse(struct worker_area *workerarea)
{
	printf("begin parse.....\n");
	ParseReqLine(workerarea);
	ParseHead(workerarea);
	ParseBody(workerarea);
	printf("recv package:\n");
	printf("|--reqLine:\n");
	printf("   |--method:%s\n",workerarea->recv.reqLine.method);
	printf("   |--url:%s\n",workerarea->recv.reqLine.url);
	printf("   |--version:%s\n",workerarea->recv.reqLine.version);
	printfHeaders(&(workerarea->recv.header));
	printf("|--body:\n%s",workerarea->recv.body);
	return 0;
}
void get(struct worker_area *workerarea)
{
	printf("int get\n");
	struct worker_recv *recv=&(workerarea->recv);
	int nbytes=1;
	int n=-1;
	unsigned long r1,r2;
	char *res=workerarea->res;
	int length=0;
	int  err=workerarea->err;//返回状态
	char *msg="OK";//状态信息
	char data[64]="";//时间信息
	char lm[64]="";//最后修改时间
	char etag[64]="";//etag标签
	long cl;//文件长度
	char range[64]="";//范围
	struct mime_type *mime=NULL;
	char *fmt="%a, %d %b %Y %H:%M:%S GMT";
	struct stat file_stat;
	/*当前时间*/
	time_t t= time(NULL);
	//printf("dermine\n");
	strftime(data,sizeof(data),fmt,localtime(&t));
	/*最后修改时间*/
	//printf("fstat\n");
	fstat(workerarea->local,&file_stat);
	strftime(lm,sizeof(lm),fmt,localtime(&(file_stat.st_mtime)));
	/*ETAG*/
	//printf("etag\n");
	snprintf(etag,sizeof(etag),"%lx.%lx",(unsigned long)(file_stat.st_mtime),(unsigned long)(file_stat.st_size));
	/*文件长度*/
	cl=(long)file_stat.st_size;
	/*mime*/
	printf("mime\n");
	mime=Mime_Type(recv->reqLine.url,strlen(recv->reqLine.url));
	/*范围range*/
	printf("format range\n");
	memset(range,0,sizeof(range));
	switch(err)
	{
		case 200:msg="OK";break;
		case 404:msg="Not found";break;
	}
	if(recv->header.range.length>0)
	{
		if((n=sscanf(recv->header.range.s,"bytes=%lu-%lu",&r1,&r2))>0)
		{
			err=206;
			lseek(workerarea->local, r1, SEEK_SET);
			if(n==2)
			{
				cl=r2-r1+1;
			}
			else
				cl-=r1;
			snprintf(range,sizeof(range),"Content-Range: bytes %lu-%lu/%lu\r\n",
					r1,
					(r1 + cl - 1),
					(unsigned long) file_stat.st_size);
			workerarea->err=err;
			msg="Partial Content";
		}
	}
	workerarea->file_length=cl;
	printf("prepare to format the response area\n");
	memset(workerarea->res,0,sizeof(workerarea->res));
	snprintf(
			workerarea->res,
			sizeof(workerarea->res),
			"%s %d %s\r\n"
			"Data: %s\r\n"
			"Last-Modified: %s\r\n"
			"Etag: \"%s\"\r\n"
			"Content-Type: %.*s\r\n"
			"Content-Length: %lu\r\n"
			"Accept-Ranges: bytes\r\n"
			"%s\r\n",
			recv->reqLine.version,err,msg,
			data,
			lm,
			etag,
			(int)strlen(mime->mime_type),mime->mime_type,
			(unsigned long)cl,
			range//为什么这个是下一行的？这里有问题。
			);
	printf("out of get\n");
}	
void post(struct worker_area *workerarea)
{
}
void put(struct worker_area *workerarea)
{
}
void trace(struct worker_area *workerarea)
{
}
void connectD(struct worker_area *workerarea)
{
}
void option(struct worker_area *workerarea)
{
}
void head(struct worker_area *workerarea)
{
}
int Request_Response(struct worker_area *workerarea)
{
	char *method=workerarea->recv.reqLine.method;
	int nbytes=0;
	if(strncmp(method,"GET",3)==0)
	{
		get(workerarea);//将reqLine和head写入dreq。
		int fd=workerarea->local;
		int len=workerarea->file_length;//文件大小
		int lenofres=strlen(workerarea->res);//空间大小
		printf("send\n");
		printf("content:%.*s\n",lenofres,workerarea->res);
		nbytes=write(workerarea->away,workerarea->res,lenofres);
		lenofres=sizeof(workerarea->res);
		if(len<lenofres)//一次发送文件够用
		{
			read(workerarea->local,workerarea->res,len);
			nbytes=write(workerarea->away,workerarea->res,len);
			printf("write bytes:%d lenth:%d\n",nbytes,len);
		}
		else
		{
			for(;len<lenofres;len-=lenofres)
			{
				read(workerarea->local,workerarea->res,lenofres);
				write(workerarea->away,workerarea->res,lenofres);
				memset(workerarea->res,0,lenofres);
			}
			read(workerarea->local,workerarea->res,len);
			write(workerarea->away,workerarea->res,len);
		}
		printf("send over\n");
	}
	else if(strncmp(method,"HEAD",4)==0)
	{
		head(workerarea);
	}
	else if(strncmp(method,"POST",4)==0)
	{
		post(workerarea);
	}
	else if(strncmp(method,"TRACE",5)==0)
	{
		trace(workerarea);
	}
	else if(strncmp(method,"CONNECT",7)==0)
	{
		connectD(workerarea);
	}
	else if(strncmp(method,"OPTION",6)==0)
	{
		option(workerarea);
	}
	return 0;
}
