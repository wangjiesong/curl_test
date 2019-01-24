#include <curl/curl.h>
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if (strlen((char *)stream) + strlen((char *)ptr) > 8192) return 0;
    strcat(stream, (char *)ptr);
    return size*nmemb;
}

int main(int argc, char **argv)
{
    int rt=0;
    char rcv_buf[8192];
    char url="http://10.12.2.8:7902/open/check.ashx";
    char send_data="datajson=\
    {\
        \"action\":\"checkid\",\
        \"params\":{\
            \"localonly\":1,\
            \"btype\":\"01\",\
            \"channelid\":\"IC\",\
            \"idname\":\"?M㈡~V规~X~N\",\
            \"idno\":\"320681198706280211\"\,\
            \"accesskey\":\"957971E9222B1D46D8059252711E0E50\"\
        }\
    }";
    /*1 全局初始化 进程启动时调用 多线程环境下只能调用一次*/
    rt = curl_global_init(CURL_GLOBAL_ALL);
    /*2 easy句柄初始化*/
    CURL *easy_handle = curl_easy_init();
    /*3 设置传输选项,传入URL,json数据报等*/
    curl_easy_setopt(easy_handle, CURLOPT_URL, url );
    curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS,send_data );
    /*4 设置的传输选项，实现回调函数以完成用户特定任务*/
    curl_easy_setopt(easy_handle,CURLOPT_WRITEFUNCTION,write_data);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, rcv_buf );
    
    
    /*curl_easy_setopt(easy_handle, CURLOPT_URL, argv[1] );
    curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, argv[2]);*/
    
    /*5 执行通信传输*/
    rt = curl_easy_perform(easy_handle);
    printf("recived:===============\n%s\n================\n", rcv_buf );
    /*6 释放内存*/
    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();

    return 0;
}

