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

#### Printing
libcurl:
```c
#include <curl/curl.h>
/*回调函数 将服务方返回的http报文体json考出*/
size_t write_rcv_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	  struct {
    	int  buf_max_len;
    	int  buf_now_len;
    	char * data_buf;
    } * rcv_data_struct=ptr;
    
    if ( rcv_data_struct->buf_now_len + size*nmemb  > rcv_data_struct->buf_max_len ) 
    	return 0;
    memcpy(rcv_data_struct->data_buf+rcv_data_struct->buf_now_len, (char *)ptr,size*nmemb);
    rcv_data_struct->buf_now_len+=size*nmemb;
    return size*nmemb;
}

/*消费方微服务框架函数封装,网络通信及http协议报文解析交有curl完成*/
int my_curl_easy_perform( char * service_id, char * send_data,int send_len, char * rcv_data,  int * rcv_len  ){
  
    
    char url[256]=""; /*例"http://10.12.2.8:7902/open/check.ashx"*/  
	  struct {
    	int  buf_max_len;
    	int  buf_now_len;
    	char * data_buf;
    } rcv_data_struct;
    
    rcv_data_struct.buf_max_len=rcv_len;
    rcv_data_struct.buf_now_len=0;
    rcv_data_struct.data_buf=rcv_data;

    /*一些curl初始化*/
    CURL *easy_handle = curl_easy_init();/*easy句柄初始化*/
    /*发送报文send_data绑定到easy_handle*/
    curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, send_data);
    /*设定返回报文的存放位置rcv_data_struct, 以及获取返回报文的回调函数write_rcv_data*/
    curl_easy_setopt(easy_handle,CURLOPT_WRITEFUNCTION,write_rcv_data);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &rcv_data_struct );
    
    /*a.检测及(如未启动)启动zk订阅线程*/
    if( check_startup_zkthread() != 0)
    	goto ERROR;
    
    /*b.服务寻址：
      检测服务目录缓存是否已存有service_id的ip/端口等url信息,
      如果存有则直接使用,如果无则通知zk线程从zookeeper注册中心获取*/
    if( get_service_addr(service_id,url)!=0)
    	goto ERROR;
    
	  /*c.流控及监控数据推送*/
	  if(floodcrl_monitorpush(easy_handle,url)!=0 )
	  	goto ERROR;
	  
	  /*d.连接服务方发送交易报文,并接收返回*/
	  curl_easy_setopt(easy_handle, CURLOPT_URL, url);/*指定服务地址URL*/
	  if( curl_easy_perform( easy_handle )!=0)
	  	goto ERROR;
	  
	  /*正确返回*/
    curl_easy_cleanup(easy_handle);
	  *rcv_len=rcv_data_struct.buf_now_len;
	  return OK;
	  
	  /*错误返回*/
	  ERROR:
	  	write_log(...);
	  	curl_easy_cleanup(easy_handle);
	  	*rcv_len=0;
	  	return ERROR;
}

int main(int argc, char **argv)
{
    int rt=0;
    char rcv_data[8192];
    char rcv_len;
    char service_id="checke_identification";
    char send_data[8192]="datajson=\
    {\
        \"action\":\"checkid\",\
        \"params\":\{\
            \"localonly\":1,\
            \"btype\":\"01\",\
            \"channelid\":\"IC\",\
            \"idname\":\"?M㈡~V规~X~N\",\
            \"idno\":\"320681198706280211\",\
            \"accesskey\":\"957971E9222B1D46D8059252711E0E50\"\
        }\
    }";

    /*1 全局初始化 进程启动时调用 多线程环境下只能调用一次*/
    rt = curl_global_init(CURL_GLOBAL_ALL);
    
    /*2 交易组报文(略),json报文保存在send_data*/
    
    /*3 服务调用*/
    rt = my_curl_easy_perform(service_id,send_data,strlen(send_data), rcv_data, &rcv_len );

    /*4 交易解报文及后继业务处理(略)，返回的json报文保存在rcv_data，报文长度rcv_len*/

    /*5 全局清理 进程退出调用 多线程环境只调用一次*/
    curl_global_cleanup();

    return 0;
}
```
