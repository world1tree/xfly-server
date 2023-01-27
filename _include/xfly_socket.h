//
// Created by zaiheshi on 1/27/23.
//

#ifndef XFLY_XFLY_SOCKET_H
#define XFLY_XFLY_SOCKET_H

#include <vector>
#include "sys/epoll.h"
#include "sys/socket.h"

#define XFLY_LISTEN_BACKLOG 511   // 以完成队列最大连接数
#define XFLY_MAX_EVENTS 512           // epoll_wait一次最多接收这么多个事件

// 处理结构体相互引用
typedef struct listening_s listening_t, *lplistening_t;
typedef struct connection_s connection_t, *lpconnection_t;
class CSocket;

// 定义成员函数指针
typedef void (CSocket::*event_handler_pt)(lpconnection_t c);

// 和监听端口有关的结构
struct listening_s {
  int port; //端口号
  int fd;   // 套接字句柄socket
  lpconnection_t connection;  // 连接池中的一个连接
};


struct connection_s{
  int fd; // 套接字句柄socket
  lplistening_t listening; // 如果这个链接被分配给了一个监听套接字，那么这个里面就指向结构首地址

  unsigned instance:1; // [位域]失效标志位, 0: 有效，1: 失效
  uint64_t iCurrsequence; //引入的一个序号，每次分配出去时+1, 可在一定程度上检测错包废包
  struct sockaddr s_sockaddr; // 保存对方地址信息

  //uint8_t r_read; //读准备好标记
  uint8_t w_ready; //写准备好标记

  // 成员函数指针
  event_handler_pt rhandler; //读事件相关处理方法
  event_handler_pt whandler; //写事件相关处理方法

  lpconnection_t data;       // 用于把空闲的连接池对象串起来构成一个单向链表
};


class CSocket {
public:
  CSocket();
  virtual ~CSocket();
  virtual bool Initialize();
public:
  int epoll_init();
  int epoll_add_event(int fd, int readevent, int writeevent, uint32_t otherflag, int eventtype, lpconnection_t c);
  int epoll_process_event(int timer);
private:
  void readConf();
  bool open_listening_sockets();                          // 监听必须的端口
  void close_listening_sockets();                         // 关闭监听套接字
  bool setnonblocking(int sockfd);                        // 设置非阻塞套接字
  // 一些业务处理函数handler
  void event_accept(lpconnection_t oldc);                 // 建立新连接
  void wait_request_handler(lpconnection_t c);            // 设置数据来时的读处理函数
  void close_accepted_connection(lpconnection_t c);       // 用户连入, accept4()时，得到的socket在处理中产生失败，则资源用这个函数释放

  // 获取对端信息
  size_t sock_ntop(struct sockaddr* sa, int port, u_char* text, size_t len); // 根据参数1给定的信息，获取地址端口字符串，返回这个字符串的长度

  // 连接池或连接相关
  lpconnection_t get_connection(int isock);               // 从连接池中获取空闲连接
  void free_connection(lpconnection_t c);                 // 归返参数c所代表的连接到连接池中
private:
  int m_worker_connections;                               // epoll连接的最大项数
  int m_listenPortCount;                                  // 监听的端口数量
  int m_epollhandle;                                      // epoll_create返回的句柄

  // 和连接池有关的
  lpconnection_t m_pconnections;                          // 连接池首地址
  lpconnection_t m_pfree_connections;                     // 空闲连接链表头

  int m_connection_n;                                     // 当前进程中所有连接对象的总数[连接池大小]
  int m_free_connection_n;                                // 连接池中可用连接总数

  std::vector<lplistening_t> m_listenSocketList;          // 监听的套接字队列

  struct epoll_event m_events[XFLY_MAX_EVENTS];                // 用于在epoll_wait()中承载返回的发生事件

};

#endif // XFLY_XFLY_SOCKET_H
