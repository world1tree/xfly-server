//
// Created by zaiheshi on 1/27/23.
//

#include "xfly_socket.h"
#include "fmtlog.h"
#include "xfly_conf.h"
#include "xfly_memory.h"
#include <arpa/inet.h>
#include <cstring>
#include <ranges>
#include <sys/ioctl.h>
#include <sys/socket.h>
CSocket::CSocket() {
  // 配置相关
  m_worker_connections = 1;
  m_listenPortCount = 1;

  // epoll相关
  m_epollhandle = -1;     // epoll返回的句柄
  m_pconnections = nullptr; // 连接池[连接数组]初始为空
  m_pfree_connections = nullptr;

  m_iLenPkgHeader = sizeof(COMM_PKG_HEADER);
  m_iLenMsgHeader = sizeof(STRUC_MSG_HEADER);
}
CSocket::~CSocket() {
  // 释放监听端口相关内存
  for (const auto& it: m_listenSocketList)
    delete it;
  m_listenSocketList.clear();
  // 连接池相关内存
  if (m_pconnections)
    delete [] m_pconnections;
  // 清空消息队列
  clearMsgRecvQueue();
}
void CSocket::clearMsgRecvQueue() {
  char* tmpMempoint;
  Memory* p_memory = Memory::getInstance();
  // 有线程池就需要考虑临界问题
  while (!m_MsgRecvQueue.empty()) {
    tmpMempoint = m_MsgRecvQueue.front();
    m_MsgRecvQueue.pop_front();
    p_memory->FreeMemory(tmpMempoint);
  }
}
bool CSocket::Initialize() {
  readConf();
  bool ret = open_listening_sockets();
  return ret;
}
void CSocket::readConf() {
  Config* conf = Config::getInstance();
  m_listenPortCount = conf->getIntDefault("ListenPortCount", m_listenPortCount);
  m_worker_connections = conf->getIntDefault("worker_connections", m_worker_connections);
}
bool CSocket::open_listening_sockets() {
  int isock;                    //socket
  struct sockaddr_in serv_addr; // 服务器的地址结构体
  int iport;                    // 端口
  char strinfo[100];            // 临时字符串

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;     // 协议族为IPV4
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本地所有IP地址，INADDR_ANY表示的是一个服务器上所有的网卡

  // 中途用到的一些配置信息
  Config* conf = Config::getInstance();
  for (int i: std::views::iota(0, m_listenPortCount)) {
    // AF_INET: 使用IPVW
    // SOCK_STREAM: 使用TCP
    // 参数3: 给0, 固定用法
    isock = socket(AF_INET, SOCK_STREAM, 0);  // 系统函数，成功返回非负描述符, 出错返回-1
    if (isock == -1) {
      loge("create socket error.");
      return false;
    }
    int reuseaddr = 1;   // 1.打开对应的设置项
    // 设置套接字参数选项
    // SOL_SOCKET: 表示级别，和参数3配套使用
    // 参数3: 允许重用本地地址
    // 设置SO_REUSEADDR目的是为了解决TIME_WAIT这个状态导致blind()失败的问题
    if (setsockopt(isock, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddr, sizeof(reuseaddr)) == -1) {
      loge("setsockopt error.");
      return false;
    }
    // 设置该socket为非阻塞
    if (!setnonblocking(isock)) {
      loge("setnonblocking error.");
      close(isock);      // 关闭套接字
      return false;
    }

    // 设置本服务器要监听的地址和端口
    strinfo[0] = 0;
    sprintf(strinfo, "ListenPort%d", i);
    iport = conf->getIntDefault(strinfo, 10000);    // default 10000
    serv_addr.sin_port = htons((in_port_t)iport);   // in_port_t就是uint16_t

    // 绑定服务器地址结构体
    if (bind(isock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
      loge("bind error.");
      close(isock);
      return false;
    }

    // 开始监听
    if (listen(isock, XFLY_LISTEN_BACKLOG) == -1) {
      loge("listen failed.");
      close(isock);
      return false;
    }

    lplistening_t item = new listening_t{iport, isock};
    logi("监听{}端口成功", iport);
    m_listenSocketList.push_back(item);
  }
  fmtlog::poll(true);
  return true;
}
void CSocket::close_listening_sockets() {
  for (int i: std::views::iota(0, m_listenPortCount)) {
    close(m_listenSocketList[i]->fd);
    logi("关闭{}端口成功", m_listenSocketList[i]->port);
  }
  fmtlog::poll(true);
}
// 设置socket为非阻塞模式; 非阻塞: 不断调用、不断调用，拷贝数据的时候是阻塞的
bool CSocket::setnonblocking(int sockfd) {
  int nb = 1; // 0清除，1设置
  if (ioctl(sockfd, FIONBIO, &nb) == 1)
    return false;
  return true;
}
// 本函数被workerProcessCycle调用
int CSocket::epoll_init() {
  // 1. 创建一个epoll对象，参数一般>0即可
  m_epollhandle = epoll_create(m_worker_connections);
  if (m_epollhandle == -1) {
    loge("epoll_create error.");
    exit(2);
  }

  // 2. 创建连接池(数组), 大小为配置文件中每个worker的最大连接数
  m_connection_n = m_worker_connections;
  m_pconnections = new connection_t[m_connection_n];
  // 把空闲数组单独链接起来，方便快速查找空闲连接
  lpconnection_t next = nullptr;
  for (int i = m_connection_n-1; i >= 0; --i) {
    m_pconnections[i].data = next;  // 指向下一个元素的指针
    m_pconnections[i].fd = -1;      // 初始化连接，暂无socket和该连接池中的连接[对象]绑定
    m_pconnections[i].instance = 1; // 失效
    m_pconnections[i].iCurrsequence = 0; // 序号统一从0开始
    next = &m_pconnections[i];      // next指针前移
  }
  m_pfree_connections = next;
  m_free_connection_n = m_connection_n;

  // 3. 遍历所有监听socket, 为每个监听socket增加一个连接池中的连接(让一个socket和一块内存绑定，以方便记录该socket相关的数据、状态等等)
  lpconnection_t c = nullptr;
  // pos是一个指针
  for (const auto& pos: m_listenSocketList) {
    c = get_connection(pos->fd);           // 监听端口占用了额外的连接
    if (c == nullptr) {
      loge("get_connection error.");
      exit(2);
    }
    // 问题是监听对象目前似乎只有2个，但是连接对象可以很多很多??? 原因是监听套接字拿到新的连接后，会调用get_connection来设置新的内存存放进入的连接
    c->listening = pos;           // 连接对象和监听对象关联, 方便通过连接对象找监听对象
    pos->connection = c;          // 监听对象和连接对象关联, 方便通过监听对象找连接对象
    //对监听端口的读事件设置处理方法，因为监听端口是用来等对方连接发送三路握手的，所以监听端口关心的就是读事件
    // 当一个新用户连入服务器的时候，就会触发event_accept函数
    c->rhandler = &CSocket::event_accept;
    //往监听socket上增加监听事件，从而开始让监听端口履行其职责(如果不加这行，虽然端口能连上，但不会触发epoll_process_events()里面的epoll_wait()往下走)
    //把感兴趣的事件加入到红黑树中，这样当监听套接字来连接的时候，系统能够通知我们
    // 当监听套接字上有读事件，比如三次握手，监听套接字上就能得到内核给我的信息
    if (epoll_add_event(pos->fd,          // socket句柄
                        1,                // 是否关注读事件
                        0,                // 是否关注写事件
                        0,                // 其它补充标记
                        EPOLL_CTL_ADD,    // 事件类型(增加、删除/修改)
                        c) == -1)         // 连接池中的连接
      exit(2);
  }
  return 1;
}
int CSocket::epoll_add_event(int fd, int readevent, int writeevent,
                             uint32_t otherflag, int eventtype,
                             lpconnection_t c) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  // 读事件
  if (readevent == 1) {
    // EPOLLIN: read only[客户端三次握手连接进来，属于一种可读事件]
    // EPOLLRDHUP: 客户端关闭连接，断连
    ev.events = EPOLLIN|EPOLLRDHUP;
    // ET: 边缘触发，拿accept来说，客户端连入时，epoll_wait只会返回一次该事件
    // LT: 水平触发, 客户端连入时，epoll_wait会被触发多次，一直到用accept来处理
//    ev.events |= (ev.events | EPOLLET);
  } else {
    // 其他事件类型待处理
  }
  if (otherflag != 0)
    ev.events |= otherflag;

  // 指针二进制最后一位一定不是0? 所以可以用它保存额外信息
  ev.data.ptr = (void*)((uintptr_t)c | c->instance);
  // eventtype=EPOLL_CTL_ADD, 把感兴趣的事件加入到红黑树中，这样当监听套接字来连接的时候，系统能够通知我们
  if (epoll_ctl(m_epollhandle, eventtype, fd, &ev) == -1) {
    loge("epoll_ctl error.");
    return -1;
  }
  return 1;
}

// 用户三次握手成功连入进来，"连入进来"这个事件对于服务器而言就是*监听套接字*上的可读事件
// 开始获取发生的事件消息
// timer: epoll_wait()阻塞的时长，单位是毫秒
// 返回值1 or 0, 不管是正常还是有问题，都应该保持进程继续运行
// 本函数被process_events_and_timers()调用, 而process_events_and_timers在子进程的死循环中被反复调用
int CSocket::epoll_process_event(int timer) {
  // 等待事件，事件会返回到m_events里，最多返回XFLY_MAX_EVENTS个事件
  // 阻塞timer这么长时间，除非a)阻塞时间到达; b)阻塞期间收到事件会立刻返回; c)调用时有事件也会立即返回; d)来个信号，比如kill -1 pid测试
  // 返回值: 错误发生返回-1, 错误在errno中
  // 如果等待一段时间，超时了，则返回0
  // 如果返回>0则表示成功捕获到这么多个事件(返回值里)
  // 从双向链表中去获取事件
  int events = epoll_wait(m_epollhandle, m_events, XFLY_MAX_EVENTS, timer);
  if (events == -1) {
    if (errno == EINTR) {
      // 信号所导致
      logi("epoll_wait failed({})", errno);
      return 1;
    } else {
      // 这被认为有问题，应该记录
      loge("epoll_wait failed({})", errno);
      return 0;
    }
  }
  if (events == 0) {
    if (timer != -1) {
      // 阻塞到时间
      return 1;
    }
    loge("epoll_wait没超时却没返回任何事件.");
    return 0;
  }

  //会惊群, 一个telnet上来，4个worker进程都会被惊动，都执行下面这个
//  logi("惊群测试: {}", events);

  // 走到这里，属于有事件收到了
  lpconnection_t c;
  uintptr_t instance;
  uint32_t revents;
  for (int i = 0; i < events; ++i) {
    c = (lpconnection_t)(m_events[i].data.ptr);   // 获取与监听套接字绑定的连接 | instance 信息
    instance = (uintptr_t)c & 1;                  //
    c = (lpconnection_t)((uintptr_t)c & (uintptr_t)~1);

    // 过期事件, 关闭连接时这个fd会被设置为-1
    // 比如用epoll_wait取得三个事件，处理第一个事件，因为业务需要, 我们把这个连接关闭，那我们应该把c->fd设置为-1
    // 第二个事件照常处理
    // 第三个事件，假如第三个事件, 也和第一个事件对应的是同一个连接，那这个条件就会成立，这种事件属于过期事件，不该处理
    if (c->fd == -1)
      continue;
    // 处理漏网的过期事件
    if (c->instance != instance)
      continue;
    // 走到这里，认为事件都没过期，正常开始处理
    revents = m_events[i].events;  // 取出事件类型
    if (revents & (EPOLLERR|EPOLLHUP)) { // 例如对方close掉套接字，这里会感应到(发生了错误或者客户端断连)
      // 加上读写标记，方便后续代码处理
      revents |= (EPOLLIN | EPOLLOUT);
    }
    // 读事件(三次握手进来, 实际执行event_accept, 因为之前rhandler已经设置了)
    if (revents & EPOLLIN) {
      // 调用类的成员函数, 最终处理的还是连接这个数据结构
      (this->*(c->rhandler))(c);
    }
    // 写事件
    if (revents & EPOLLOUT) {
    }
  }
  return 1;
}
