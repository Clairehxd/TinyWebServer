#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>         //包含了许多系统服务的函数原型，例如read函数、write函数和getpid函数等
#include <signal.h>         //定义了一个变量类型 sig_atomic_t、两个函数调用和一些宏来处理程序执行期间报告的不同信号。
#include <sys/types.h>      //是Unix/Linux系统的基本系统数据类型的头文件，含有size_t，time_t，pid_t等类型。
#include <sys/epoll.h>      //在linux的网络编程中，很长的时间都在使用select来做事件触发。在linux新的内核中，有了一种替换它的机制，就是epoll。相比于select，epoll最大的好处在于它不会随着监听fd数目的增长而降低效率。
#include <fcntl.h>          //访问权限、创建文件模式、非阻塞标记
#include <sys/socket.h>     //构建socket;获取socket选项;设置socket选项;bind;监听指定socket;accept;连接socket;发送消息;接收消息
#include <netinet/in.h>     //socketaddr_in 结构体;htons系统调用
#include <arpa/inet.h>      //inet_pton;
#include <assert.h>         //断言
#include <sys/stat.h>       //是unix/linux系统定义文件状态所在的伪标准头文件。
#include <string.h>         //字符串
#include <pthread.h>        //线程
#include <stdio.h>          //I/O
#include <stdlib.h>         //系统函数
#include <sys/mman.h>       //内存管理声明
#include <stdarg.h>         //主要目的为让函数能够接收可变参数。
#include <errno.h>          //定义了通过错误码来回报错误资讯的宏。
#include <sys/wait.h>       //使用wait()和waitpid()函数时需要include这个头文件。wait()会暂时停止目前进程的执行，直到有信号来到或子进程结束。waitpid()会暂时停止目前进程的执行，直到有信号来到或子进程结束。
#include <sys/uio.h>        //definitions for vector I/O operations
#include <map>              //有序哈希

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;            //文件名长度
    static const int READ_BUFFER_SIZE = 2048;       //读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;      //写缓冲区大小
    enum METHOD                          //http网络请求方法
    {
        GET = 0,        //get请求是用来获取数据的，只是用来查询数据，不对服务器的数据做任何的修改，新增，删除等操作。   安全、幂等      
        POST,           //post请求一般是对服务器的数据做改变，常用来数据的提交，新增操作。post请求参数都在body请求体中，body大小是有限制的
        HEAD,           //HEAD和GET本质是一样的，区别在于HEAD不含有呈现数据，而仅仅是HTTP头信息。有的人可能觉得这个方法没什么用，其实不是这样的。想象一个业务情景：欲判断某个资源是否存在，我们通常使用GET，但这里用HEAD则意义更加明确。
        PUT,            //put请求与post一样都会改变服务器的数据，但是put的侧重点在于对于数据的修改操作，但是post侧重于对于数据的增加。
        DELETE,         //delete请求用来删除服务器的资源。
        TRACE,          //让web服务器将之前的请求通信返回给客户方法
        OPTIONS,        //options请求属于浏览器的预检请求，查看服务器是否接受请求，预检通过后，浏览器才会去发get，post，put，delete等请求。至于什么情况下浏览器会发预检请求，浏览器会会将请求分为两类，简单请求与非简单请求，非简单请求会产生预检options请求。
        CONNECT,        //要求在与代理服务器通信时建立隧道，实现用隧道协议进行TCP通信。主要使用SSL（安全套接层）和TLS（传输层安全）协议把通信内容加密后经网络隧道传输
        PATH
    };
    enum CHECK_STATE    //主状态机
    {
        CHECK_STATE_REQUESTLINE = 0,    //当前正在分析请求行
        CHECK_STATE_HEADER,             //当前正在分析头部字段
        CHECK_STATE_CONTENT             //当前正在分析内容
    };
    enum HTTP_CODE      //请求结果
    {
        NO_REQUEST,         //请求不完整， 需要继续读取客户端数据
        GET_REQUEST,        //获得了完整的客户请求
        BAD_REQUEST,        //客户请求有语法错误
        NO_RESOURCE,        //客户对资源没有足够的访问权限
        FORBIDDEN_REQUEST,  //禁止客户请求
        FILE_REQUEST,       //文件请求
        INTERNAL_ERROR,     //服务器内部错误
        CLOSED_CONNECTION   //客户端已关闭连接
    };
    enum LINE_STATUS    //从状态机，行的读取状态
    {
        LINE_OK = 0,    //读到完整行
        LINE_BAD,       //行出错
        LINE_OPEN       //行数据不完整
    };

public:
    http_conn() {}      //构造函数和析构函数
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);   //初始化
    void close_conn(bool real_close = true);        //关闭conn
    void process();                 //进程
    bool read_once();               //读一次
    bool write();           //写
    sockaddr_in *get_address()          //获取地址
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);//数据库连接初始化?;client段的connection pool：连接池主要用来管理客户端的连接,避免重复的连接/断开操作,而是将空闲的连接缓存起来,可以复用。
    int timer_flag;     //时间标志
    int improv;         //?


private:
    void init();        //初始化
    HTTP_CODE process_read();   //进程读，返回值为枚举类型的其中一种
    bool process_write(HTTP_CODE ret);  //进程写
    HTTP_CODE parse_request_line(char *text);   //对读取的请求行进行解析
    HTTP_CODE parse_headers(char *text);        //对头部字段进行解析
    HTTP_CODE parse_content(char *text);        //对头部字段进行解析
    HTTP_CODE do_request();                     //请求
    char *get_line() { return m_read_buf + m_start_line; }; //得到行
    LINE_STATUS parse_line();                               //对行解析
    void unmap();                                           //?
    bool add_response(const char *format, ...);         //增加响应
    bool add_content(const char *content);          //增加内容
    bool add_status_line(int status, const char *title);    //增加状态行
    bool add_headers(int content_length);   //增加头部字段
    bool add_content_type();        //增加内容类型
    bool add_content_length(int content_length);    //增加内容长度
    bool add_linger();              //?
    bool add_blank_line();      //增加空白行

public:
    static int m_epollfd;   //高并发事件处理机制
    static int m_user_count;//计数
    MYSQL *mysql;
    int m_state;  ////读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;              //此数据结构用做bind、connect、recvfrom、sendto等函数的参数，指明地址信息。但一般并不直接针对此数据结构操作，而是使用另一个与sockaddr等价的数据结构
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
