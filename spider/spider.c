#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <regex.h>
#include <dirent.h>


#ifndef _SPIDER_H_
#define _SPIDER_H_
#define TMPSIZE 1024
#define BUFSIZE 1500
#define RESHEAD 8192
#define WEBPORT 80
#define IPSIZE 16
#define TRUE 1
#define FALSE 0
#define DOMAIN 128
#define MAX_LEN 256
#define TIMEOUT 3
typedef struct node{
	char url[TMPSIZE];
	char title[TMPSIZE];
	char domain[DOMAIN];
	char ip[IPSIZE];
	char name[TMPSIZE];
	char reshead[RESHEAD];
	int  port;
	int status;
}url_t;
typedef struct queue{
	url_t * data;/*队列成员*/
		int front;/*头索引*/
		int rear;/*尾索引*/
		int maxsize;/*队列最大数*/
		int size;/*当前使用数*/
}queue_t;
int ERR_STR(char * ,int );
int Queue_Create(queue_t *,int );
int Queue_if_full(queue_t *);
int Queue_if_empty(queue_t *);
int Queue_EN(queue_t * ,url_t);
int Queue_DE(queue_t * , url_t *);
url_t * Queue_Show(queue_t *,int);
void URL_INIT(url_t *);
char* Load_Config(const char * path,const char * key);
int Write_Config(char *path , char *section,char *key,char * value);
int Analytical_URL(url_t *);
int Get_IP(url_t *,char *);
int Connect(url_t *);
char * Create_HTTPHEAD(url_t *);
int Save_ResHead(url_t*,char*,int);
int Save_ResDate(int,url_t*);
int Analytical_HTML(char *,url_t*,queue_t*);
int Is_Path_Esist(char *path);
#endif


int Is_Path_Esist(char *path)
{
	char tmp[MAX_LEN];
	int i=0,j=0;
	DIR* errcode;
	int filecode;
	int returncode=TRUE;
	char nametmp[MAX_LEN];
	char path_tmp[MAX_LEN];
	trayagain:
	bzero(nametmp,MAX_LEN);
	j=0;
	while(*path!='/'&& *path!='\0')
	{
		tmp[i]=*path;
		nametmp[j]=*path;
	path++;
	i++;
	j++;
	}
	if(*path=='/')
	{
		tmp[i]=*path;
		tmp[++i]=0;
		path++;
		
		
		errcode=opendir(tmp);
		if(errcode==NULL)
		{
		returncode=FALSE;
		mkdir(tmp,0775);
		}
	goto trayagain;	
	}
	else
	{
			
		nametmp[j]=0;
		strcpy(path_tmp,tmp);
		filecode=access(path_tmp,F_OK);
		if(filecode!=0)
		{
			filecode=open(path_tmp,O_CREAT,0775);
			close(filecode);
			returncode=FALSE;
		}


	}


return returncode;
}
int ERR_STR(char * str,int err){
	perror(str);
	return err;
}
int Queue_Create(queue_t * p,int maxsize){
	p->data = (url_t *)malloc(sizeof(url_t)*maxsize);
	if(p->data == NULL)
		ERR_STR("Queue_Create Call Failed",-1);
	p->front=0;
	p->rear=0;
	p->size=0;
	p->maxsize = maxsize;
	return 0;
}
int Queue_if_full(queue_t *p){
	if(p->front == (p->rear+1) % p->maxsize)
		return TRUE;
	else
		return FALSE;
}
int Queue_if_empty(queue_t *p){
	if(p->front == p->rear)
		return TRUE;
	else
		return FALSE;
}
int Queue_EN(queue_t * p,url_t u)
{
	if(Queue_if_full(p))
		return FALSE;
	else{
		p->data[p->rear] = u;
		p->rear = (p->rear+1)%p->maxsize;
		return TRUE;
	}
}
int Queue_DE(queue_t*p,url_t * u)
{
	if(Queue_if_empty(p))
		return FALSE;
	else{
		*u = p->data[p->front];
		p->front = (p->front+1)%p->maxsize;
		return TRUE;
	}
}
url_t * Queue_Show(queue_t *p,int flags)
{
	int i = p->front;
	if(Queue_if_empty(p)){
		printf("The Queue is Empty\n");
		return NULL;
	}
	if(flags == 0){
		printf("SPIDER URL QUEUE START:\n");
		while(i!= p->rear){
			printf("URL:%s\tSTATUS:%d\n",p->data[i].url,p->data[i].status);
			i++;
		}
		printf("PRINT END\n");
		return NULL;
	}else{
		while(i!= p->rear){
			if((p->data[i].status)==0)
				return &(p->data[i]);
			else
				i++;
		}
		return NULL;
	}
}
void URL_INIT(url_t * u)
{
	bzero(u,sizeof(u));
	u->port=WEBPORT;
}
char * Load_Config(const char * path,const char * key)
{
	FILE * fd;
	char * str;
	char * p;
	char * value;
	ssize_t readlen;
	size_t len;
	fd = fopen(path,"r+");
	while((readlen = getline(&str,&len,fd))!=-1)
	{
		if(str[0]==';'||str[0]=='#')
			continue;
		if(p = strstr(str,key)){
			strtok(p,"=");
			value = strtok(NULL,"=");
			return value;
		}
	}
	return NULL;
}
int Analytical_URL(url_t * u){
	//http or https://news.sina.com.cn:70/tmp/cs/2017-9-11.html
	int j=0;
	int start;
	char *arr[]={"http://","https://",NULL};
	for(int i = 0;arr[i];i++)
		if(strncmp(u->url,arr[i],strlen(arr[i])))
			start = strlen(arr[i])-1;
	for(int i=start;u->url[i]!='/' && u->url[i]!='\0';i++,j++)
		u->domain[j] = u->url[i];
	u->domain[j]='\0';
	j=0;
	for(int i = start;u->url[i]!='\0';i++,j++)
		u->name[j] = u->url[i];
	u->name[j]='\0';
	char * por;
	por = strstr(u->domain,":");
	if(por){
		sscanf(por,":%d",u->port);
	}
	for(int i = 0;i<(int)strlen(u->domain);i++){
		if(u->domain[i]==':')
			u->domain[i]='\0';
	}
	Get_IP(u,u->domain);
}
int Get_IP(url_t * u,char * domain){
	struct hostent * ent;
	if((ent = gethostbyname(domain))==NULL)
		return FALSE;
	else{
		inet_ntop(AF_INET,ent->h_addr_list[0],u->ip,IPSIZE);
		return TRUE;
	}
}
int Connect(url_t * u){
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
		return FALSE;
	int res,flag=3;
	struct sockaddr_in addr;
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(u->ip);
	addr.sin_port = htons(u->port);
tryagain:
	res = connect(sockfd,(struct sockaddr*)&addr,sizeof(addr));
	if(res == -1){
		printf("Connect ERROR:TryAgain Network connection IP:%s\tURL:%s\tCOUNT:%d\n",u->ip,u->domain,flag--);
		if(flag == 0)
			return FALSE;
		sleep(1);
		goto tryagain;
	}
	return sockfd;
}
char * Create_HTTPHEAD(url_t * u){
	char headr[TMPSIZE];
	char * tmp;
	bzero(headr,sizeof(headr));
	sprintf(headr,
			"GET %s HTTP/1.1\r\n"\
			"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
			"User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
			"Host:%s\r\n"\
			"Connection:close\r\n"\
			"\r\n"\
			,u->url,u->domain);
	if((tmp = malloc(sizeof(char)*strlen(headr)))==NULL){
		printf("Create_HTTPHEAD ERROR:Malloc Call Failed\n");
		return NULL;
	}
	strncpy(tmp,headr,strlen(headr));
	return tmp;
}
int Save_ResHead(url_t * u,char * headr,int sockfd){
	ssize_t redlen;
	int flags,j=0;
	char buf[RESHEAD];
	char RES[RESHEAD];
	bzero(buf,sizeof(buf));
	bzero(RES,sizeof(RES));
	if((write(sockfd,headr,strlen(headr)))==-1)
		return FALSE;
	while((redlen = read(sockfd,buf,1))!=0){
		flags=0;
		RES[j]=buf[0];
		for(int i = strlen(RES)-1;RES[i]=='\n'||RES[i]=='\r';i--,flags++);
		if(flags == 4)
			break;
		j++;
	}
	if(strlen(RES)==0)
		return FALSE;
	else{
		strncpy(u->reshead,RES,strlen(RES));
		free(headr);
		return TRUE;
	}
}
int Save_ResDate(int sockfd,url_t*u)
{
	int fd;
	ssize_t redlen;
	char buf[RESHEAD];
	bzero(buf,sizeof(buf));
	umask(0);
	int is=Is_Path_Esist(u->name);
	if(is==TRUE)
		return TRUE;
	if((fd = open(u->name,O_WRONLY))==-1){
		printf("Save_ResDate ERROR:Open Call Failed\n");
		return FALSE;
	}
	while((redlen = read(sockfd,buf,sizeof(buf)))!=0){
		write(fd,buf,redlen);
		bzero(buf,sizeof(buf));
	}
	close(fd);
	close(sockfd);
	return TRUE;
}
int Analytical_HTML(char * path,url_t * u,queue_t * t){
	char * p;
	char buf[TMPSIZE];
	char url[TMPSIZE];
	int filesize,fd,errcode;
	url_t tmp;
	regmatch_t tm[2];
	regex_t reg;
	if((fd = open(path,O_RDONLY))==-1){
		printf("Analytical_HTML:Open Call Failed\n");
		return FALSE;
	}
	if((filesize = lseek(fd,0,SEEK_END))==-1){
		printf("Analytical_HTML:Lseek Call Failed\n");
		return FALSE;
	}
	if((p = mmap(NULL,filesize,PROT_READ,MAP_SHARED,fd,0))==NULL){
		printf("Analytical_HTML:Mmap Call Failed\n");
		return FALSE;
	}
	close(fd);
	/*regex 族*/
	errcode = regcomp(&reg,"<a[^>]\\+\\?href=\"\\([^\"]\\+\\?.shtml\\)\"[^>]\\+\\?>\\([^<]\\+\\?\\)</a>",0);
		if(errcode != 0){
			regerror(errcode,&reg,buf,sizeof(buf));
			printf("Analytical_HTML Regcomp ERROR:%s\n",buf);
			return FALSE;
		}
	while((errcode = regexec(&reg,p,2,tm,0))==0){
		bzero(url,sizeof(url));
		snprintf(url,(tm[1].rm_eo)-(tm[1].rm_so)+1,"%s",p+tm[1].rm_so);
		while(1){
			URL_INIT(&tmp);
			if((Queue_if_full(t))!=TRUE){
				strncpy(tmp.url,url,strlen(url));
				Queue_EN(t,tmp);
				break;
			}else{
				sleep(1);
				continue;
			}
		}
		p+=tm[0].rm_eo;
	}
	if(errcode == REG_NOMATCH){
		regfree(&reg);
		munmap(p,filesize);
		u->status=1;
		return TRUE;
	}else{
		regerror(errcode,&reg,buf,sizeof(buf));
		printf("Analytical_HTML Regexec ERROR:%s\n",buf);
		return FALSE;
	}

}
int main(void)
{
	/*未处理已处理队列*/
	queue_t treated,handled;
	url_t url,tmp;
	url_t * tp;
	URL_INIT(&url);
	URL_INIT(&tmp);
	char * value = Load_Config("./Spider.ini","URL");
	char * head;
	int sockfd;
	strncpy(url.url,value,strlen(value)-1);
	Queue_Create(&treated,100);
	Queue_Create(&handled,500);
	Queue_EN(&treated,url);
	Queue_Show(&treated,FALSE);
	Queue_DE(&treated,&tmp);
	Analytical_URL(&tmp);
	printf("tmp->url:%s\ndomain:%s\nname:%s\nport:%d\nip:%s\n",
			tmp.url,
			tmp.domain,
			tmp.name,
			tmp.port,
			tmp.ip);
	head = Create_HTTPHEAD(&tmp);
	printf("\nHEAD:%s\n",head);
	sockfd = Connect(&tmp);
	Save_ResHead(&tmp,head,sockfd);
	printf("RESHEAD:%s\n",tmp.reshead);
	Save_ResDate(sockfd,&tmp);
	Queue_EN(&handled,tmp);
	Queue_Show(&handled,FALSE);
	tp = Queue_Show(&handled,TRUE);
	Analytical_HTML(tmp.name,tp,&treated);
	Queue_Show(&treated,FALSE);
	Queue_Show(&handled,FALSE);
	return 0;
}

