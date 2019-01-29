# curl_test



```c
#include <cjson/cJSON.h>
```

### Data Structure

cJSON represents JSON data using the `cJSON` struct data type:

```c
/* The cJSON structure: */
typedef struct cJSON
{
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
```



```json
{
    "name": "Awesome 4K",
    "resolutions": [
        {
            "width": 1280,
            "height": 720
        },
        {
            "width": 1920,
            "height": 1080
        },
        {
            "width": 3840,
            "height": 2160
        }
    ]
}
```

#### Printing
Let's build the above JSON and print it to a string:
```c
const int ig_timeout=60; /*通信超时时间*/
const int lg_port=6088;  /*服务端监听端口*/
const int ig_max_fork=100; /*进程池进程个数*/

void worker( const int i_srv_fd )
{
   int i_fd;
   int in_len;
   int ret_len;
   char in_buf[MAX_BUF_LEN];
   char out_buf[MAX_BUF_LEN];
   
   for ( ; ; )
   {
      /*阻塞接收主机通讯包*/
      i_fd=my_accept(i_srv_fd );
      /*读客户端数据*/
      in_len=read_data(i_fd, in_buf, sizeof(in_buf), ig_timeout);
      /*业务处理*/
      bussyness( in_buf, in_len, out_buf, &ret_len, ig_timeout );
      /*返回客户端数据*/
      write_data(i_fd, out_buf, ret_len, ig_timeout);
      /*关闭accept到的连接*/
      close(i_fd);
   }
}

int main( int argc, char *argv[])
{
   int i,j;
   int i_st;
   pid_t pid;    

   /*建立端口监听*/
   if( -1 == (i_srv_fd = my_listen(lg_port)) )
   {
      write_log( _NTERR_, "init_server error , exit");
      return;
   }
   for (;;)
   {
   	  pid=fork();
      switch( pid )
      {
         case -1:
           write_log(_NTERR_, "fork process error ");
           break;
         case  0:
           worker( i_srv_fd );
           exit(0);
           break;
         default: /*父进程*/
         	 sleep(1);
         	 i++;
           break;
      }
      /*如果有业务进程退出就会新fork一个补充，维持进程池中进程总个数*/
      if(i>=ig_max_fork) 
      {
      	   outpid = wait(&i_st);
           write_log( _NTTRC_, "child process[pid=%d] exit, start new one later" , outpid );
           i--;
      }
   }
}
```

#### Printing
libevnet:
```c
const int ig_timeout=60; /*通信超时时间*/
const int lg_port=6088;  /*服务端监听端口*/
const int ig_max_fork=100; /*进程池进程个数*/
/*流量控制和监控推送*/
int floodcrl_monitorpush(char * in_buf, int in_len, char * out_buf, int * out_len);
/*外部业务处理函数*/
int user_busyness( void * in_buf, in_len, void * out_buf,  int * ret_len );

/*封装的微服务方句柄处理函数*/
void my_httpserver_handler(struct evhttp_request *req,  
int ( * busyness )( void * in_buf, in_len, void * out_buf,  int * ret_len ) )
{
	struct evkeyvalq *headers=NULL;
	struct evkeyval *header=NULL;
	struct evbuffer *ev_buf=NULL;
	char * in_buf =NULL;
	char * out_buf[8192]="";
	int out_len;
	int http_content_len=0;
  /*取http头*/
	headers = evhttp_request_get_input_headers(req);
	/*获取http报文体长度*/
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
	    if( strcmp(header->key,"content_length")==0 ){
	    	http_content_len=atoi(header->value);
	    	break;
	    }
	}
  in_buf=(char*)malloc( http_content_len+1 );
	ev_buf = evhttp_request_get_input_buffer(req);
	/*读出http报文体*/
	evbuffer_remove(ev_buf, in_buf, http_content_len );
	
	/*流控措施、监控数据推送 不满足流控规则直接返回*/
  if( floodcrl_monitorpush(in_buf, http_content_len, out_buf, &out_len) !=0)
  	  goto return_to_client;
  	  
  /*执行业务函数*/
  out_len=sizeof(out_buf);
	busyness(in_buf, http_content_len, out_buf, &out_len);
	
  /*返回客户端*/
  return_to_client:
	evhttp_send_reply(req, 200, "OK", out_buf);
	/*释放缓冲 同时关闭连接*/
	evbuffer_free(ev_buf);
	free(in_buf);
}

void worker( const int i_listen_fd )
{
   int i_accepted_fd;
   int in_len;
   int ret_len;
   char in_buf[MAX_BUF_LEN];
   char out_buf[MAX_BUF_LEN];
   socklen_t i_socklen;
   struct  sockaddr_in  cliaddr;
   struct event_base *base = NULL;
   struct evhttp *httpd = NULL;
   
   base=event_init();
   
   for ( ; ; )
   {
   	  httpd=evhttp_new(base);
      /*阻塞接收主机通讯包*/
      evhttp_accept_socket(httpd, i_listen_fd);
      /*设置回调函数*/
      evhttp_set_gencb(httpd, my_httpserver_handler, user_busyness );
      /*事件驱动处理业务逻辑*/
      event_base_dispatch(httpd);
      /*释放事件*/
      event_free(&httpd);
   }
}

int main( int argc, char *argv[])
{
   int i,j;
   int i_listen_fd;
   pid_t pid;    

   /*建立端口监听*/
   if( -1 == (i_listen_fd = my_listen(lg_port)) )
   {
      write_log( _NTERR_, "my_listen error , exit");
      return;
   }
   for (;;)
   {
   	pid=fork();
      switch( pid )
      {
         case -1:
           write_log(_NTERR_, "fork process error ");
           break;
         case  0:
           worker(i_listen_fd );
           exit(0);
           break;
         default: /*父进程*/
         	 sleep(1);
         	 i++;
           break;
      }
      /*创建完线程池后阻塞在my_wait，
      如果有业务进程退出就会新fork一个补充，维持进程池中进程总个数*/
      if(i>=ig_max_fork) 
      {
      	 outpid = my_wait(&i_st);
           i--;
      }
   }
}
```
