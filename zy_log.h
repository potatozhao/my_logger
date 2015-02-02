/*
 * =====================================================================================
 *
 *       Filename:  logger.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/21/2015 02:53:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZHAOYANG, 
 *   Organization:  
 *
 * =====================================================================================
 */


#ifndef ZY_LOG_H_
#define ZY_LOG_H_


#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/time.h>
using namespace std;

namespace zylog{
#ifdef __cplusplus
extern "C" {
#endif
#define TIME_NOW_BUF_SIZE 1024

#define FORMAT_LOG_BUF_SIZE 4096

#define LOG_INIT(lvl,path) \
    Log.init(lvl, path)

#define LOG(lvl, ...)  \
    Log.Log(lvl, __LINE__, __func__, __VA_ARGS__)

#define LOG_ERROR(...)  \
    Log.Log(ML_ERROR, __LINE__, __func__, __VA_ARGS__)

#define LOG_WARN(...)  \
    Log.Log(ML_WARN, __LINE__, __func__, __VA_ARGS__)

#define LOG_INFO(...)  \
    Log.Log(ML_INFO, __LINE__, __func__, __VA_ARGS__)

#define LOG_DEBUG(...)  \
    Log.Log(ML_DEBUG, __LINE__, __func__, __VA_ARGS__)
#ifdef __cplusplus
}
#endif

enum m_log_level{ML_DEBUG = 0, ML_INFO, ML_WARN, ML_ERROR, ML_INVALID};

class logger
{
public:
    logger();
    ~logger();
    int init(enum m_log_level lvl,string& log_path_str);
    int set_log_level(enum m_log_level lvl);
    m_log_level get_log_level();
    int log_message(enum m_log_level lvl, int line, const char* func_name, const char* format, ...);
    int Log(enum m_log_level lvl, int line, const char* func_name, const char* format, ...);
    static void free_buffer(void* p);
    char* get_ts_data(pthread_key_t& key, int size);
    char* get_time_buff(){return get_ts_data(time_now_buffer, TIME_NOW_BUF_SIZE);};
    char* get_format_log_buffer(){return get_ts_data(format_log_msg_buffer,FORMAT_LOG_BUF_SIZE);};
    const char* time_now(char* now_str);

private:
    enum m_log_level m_level;
    pthread_key_t time_now_buffer;
    pthread_key_t format_log_msg_buffer;

    FILE* log_stream;
};

logger::logger()
{
    m_level = ML_ERROR;
    pthread_key_create(&time_now_buffer, free_buffer);
    pthread_key_create(&format_log_msg_buffer, free_buffer);
    log_stream = NULL;
}

logger::~logger()
{
    if(log_stream)
        fclose(log_stream);
}

int logger::init(enum m_log_level lvl, string& log_path_str)
{
    m_level = lvl;
    log_stream = fopen(log_path_str.c_str(),"a+");
    if (log_stream == NULL)
        return -1;
    return 0;
}

int logger::set_log_level(enum m_log_level lvl)
{
    m_level = lvl;
    return 0;
}

m_log_level logger::get_log_level()
{
    return m_level;
}
int logger::Log(enum m_log_level lvl, int line, const char* func_name, const char* format, ...)
{
    if(lvl >= m_level)
    {
        va_list va;
        char buff[FORMAT_LOG_BUF_SIZE];
        va_start(va,format);
        vsnprintf(buff, FORMAT_LOG_BUF_SIZE, format, va);
        va_end(va);
        log_message(lvl, line, func_name, buff);
        return 0;
    }
    return -1;
}

const char* logger::time_now(char* now_str)
{
    struct timeval tv;
    struct tm lt;
    time_t now = 0;
    size_t len = 0;
    gettimeofday(&tv,0);
    now = tv.tv_sec;
    localtime_r(&now, &lt);

    //"yyyy-MM-dd HH:mm:ss,SSS"
    len = strftime(now_str, TIME_NOW_BUF_SIZE, "%Y-%m-%d %H:%M:%S", &lt);
    len += snprintf(now_str + len, TIME_NOW_BUF_SIZE - len, ",%03d",(int)(tv.tv_usec/1000));
    return now_str;
}

char* logger::get_ts_data(pthread_key_t& key, int size)
{
    char* p = (char*)pthread_getspecific(key);
    if(p == 0)
    {
        int res;
        p = new char[size];
        res = pthread_setspecific(key,p);
        if(res!=0)
        {
            fprintf(stderr, "Failed to set TSD key: %d",res);
        }
    }
    return p;
}

void logger::free_buffer(void* p)
{
    p = (char*)p;
    if(p)
        delete [] p;
}

int logger::log_message(enum m_log_level lvl, int line, const char* func_name, const char* format, ...)
{
    const char* lvl2str[]={"DEBUG","INFO","WARN","ERROR","INVALID"};
    pid_t pid = 0;
    va_list va;
    int ofs = 0;
    unsigned long int tid = 0;
    const char* time = time_now(get_time_buff());
    char* buff = get_format_log_buffer();
    if(!buff)
    {
        fprintf(stderr, "log_message: Unable to allocate memory buffer\n");
        return -1;
    }

    if(pid==0)
    {
        pid = getpid();
    }

    tid = (unsigned long int)(pthread_self());
    ofs = snprintf(buff, FORMAT_LOG_BUF_SIZE-1, "%s:%d(0x%lx):%s@%s@%d: ", time, pid, tid,lvl2str[lvl], func_name, line);
    va_start(va,format);
    vsnprintf(buff+ofs, FORMAT_LOG_BUF_SIZE-1-ofs, format, va);
    va_end(va);
    if(false)
    {

    }
    else
    {
        fprintf(log_stream, "%s\n", buff);
        fflush(log_stream);
    }
}

logger Log;

}
#endif

/*
using namespace zylog;

void* threadfunc(void* lg)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    //logger* log_t = (logger*)lg;
    int a = 1000;
    while(a)
    {
        LOG_ERROR("this is a test message :%d", a);
        //log_t->log_message(ML_ERROR, __LINE__, __func__, "this is a test message :%d", a);
        select(0, NULL, NULL, NULL, &timeout);
        a--;
    }
    return 0;
}

int main()
{
    // zylog::logger lg;
    string path_str("./log");
    //lg.init(zylog::ML_ERROR,path_str);
    LOG_INIT(ML_ERROR,path_str);
    int a = 100;
    pthread_t ntid[a];
    while(a)
    {
        int err;
        err = pthread_create(&(ntid[a-1]), NULL, threadfunc, NULL);
        //lg.log_message(zylog::ML_ERROR, __LINE__, __func__, "this is a test message :%d", a);
        LOG(ML_ERROR,"this is a test message :%d", a);
        a--;
    }
    //sleep();
    pthread_join(ntid[0],NULL);
    sleep(1);
    return 0;
}*/
