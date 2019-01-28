#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

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
