//
// Created by zaiheshi on 1/27/23.
//

#include "xfly_socket.h"
#include "sys/socket.h"
#include "unistd.h"
#include "errno.h"
#include "fmtlog.h"
// 建立新连接专用函数，当新连接进入时，本函数会被epoll_process_events调用
// 这个函数一次只接入一个连接
void CSocket::event_accept(lpconnection_t oldc) {
  // 因为套接字上用的不是ET(边缘触发), 而是LT(水平触发), 意味着客户端连入如果我要不处理，这个函数会被多次调用，所以，这里可以不必多次accept()
  // 可以只执行一次accept()
  struct sockaddr mysockaddr;      // 远端服务器的socket地址
  socklen_t socklen;
  int err;
  int level;
  int s;
  static int use_accept4 = 1;      // 先认为能够使用accept4函数
  lpconnection_t newc;             // 代表连接池中的一个连接
  socklen = sizeof(mysockaddr);
  while (true) {
    if (use_accept4)
      // 从内核中获取一个用户端连接，最后一个参数SOCK_NONBLOCK表示返回一个非阻塞的socket, 节省一次ioctl(设置为非阻塞)调用
      // accept4可以认为基本解决惊群问题，但似乎并没有完全解决，有时候还会惊动其他的worker进程
      // *accept能力是从已完成队列中取出连接*, 当三路握手结束，用accept取出
      s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
    else
      s = accept(oldc->fd, &mysockaddr, &socklen);

    if (s == -1) {
      err = errno;

      // 对于accept, send, recv而言，事件还未发生时errno通常被设置成EAGAIN(再来一次) 或者 EWOULDBLOCK(期待阻塞)
      if (err == EAGAIN)
        // 除非用一个循环不断的accept()取走所有的连接，不然一般不会有这个错误(这里只取一个连接)
        return;
      // ECONNABORTED错误则发生在对方意外关闭套接字后(您的主机中的软件放弃了一个已建立的连接--由于超时或者其它失败而中止接连, 用户插拔网线就可能有这个错误)
      if (err == ECONNABORTED) {
        //该错误被描述为“software caused connection abort”，即“软件引起的连接中止”。原因在于当服务和客户进程在完成用于 TCP 连接的“三次握手”后，
        //客户 TCP 却发送了一个 RST （复位）分节，在服务进程看来，就在该连接已由 TCP 排队，等着服务进程调用 accept 的时候 RST 却到达了。
        //POSIX 规定此时的 errno 值必须 ECONNABORTED。源自 Berkeley 的实现完全在内核中处理中止的连接，服务进程将永远不知道该中止的发生。
        //服务器进程一般可以忽略该错误，直接再次调用accept。
      }

      //EMFILE:进程的fd已用尽【已达到系统所允许单一进程所能打开的文件/套接字总数】。可参考：https://blog.csdn.net/sdn_prc/article/details/28661661   以及 https://bbs.csdn.net/topics/390592927
      //ulimit -n ,看看文件描述符限制,如果是1024的话，需要改大;  打开的文件句柄数过多 ,把系统的fd软限制和硬限制都抬高.
      //ENFILE这个errno的存在，表明一定存在system-wide的resource limits，而不仅仅有process-specific的resource limits。按照
      // 常识，process-specific的resource limits，一定受限于system-wide的resource limits。
      else if (err == EMFILE || err == ENFILE) {

      }
      // 无accept4函数, 改用accept函数
      if (use_accept4 && err == ENOSYS) {
        use_accept4 = 0;
        continue;
      }
      // 对方关闭套接字
      if (err == ECONNABORTED) {

      }

      if (err == EMFILE || err == ENFILE) {
        //do nothing，这个官方做法是先把读事件从listen socket上移除，然后再弄个定时器，定时器到了则继续执行该函数，但是定时器到了有个标记，会把读事件增加到listen socket上去；
      }
      return ;
    }
    // 走到这里，表示accept4() or accept()成功了
    // 注意: 新连接用新的内存绑定，而不是用监听套接字里的连接!!!
    newc = get_connection(s);
    if (newc == nullptr) {
      if (close(s) == -1)
        loge("event_accept, get_connection failed.");
      return;
    }
    // ... 将来这里会判断是否连接超过最大允许连接数


    // 成功拿到了连接池中的一个连接
    memcpy(&newc->s_sockaddr, &mysockaddr, socklen); // 拷贝客户端地址到连接对象[要转成字符串ip地址参考函数sock_ntop]
//    u_char ipaddr[100];
//    memset(ipaddr, 0, sizeof(ipaddr));
//    sock_ntop(&newc->s_sockaddr, 1, ipaddr, sizeof(ipaddr)-10);    // 宽度给小数点
//    logi("ip信息为: {}", (const char*)ipaddr);

    if (!use_accept4) {
      // 如果不是用accept4取得的socket, 那么就要设置为非阻塞
      if (!setnonblocking(s)) {
        loge("setnonblocking failed.");
        close_accepted_connection(newc);
        return;
      }
    }

    newc->listening = oldc->listening;      // 连接对象和监听对象关联，方便通过连接对象找监听对象
    newc->w_ready = 1;                      // 标记可以写， 新连接写事件一定是ready的
    // 这里是用户连接套接字，非监听套接字，因此rhandler不一样
    // 这里的rhandler是用来接受客户端信息的
    newc->rhandler = &CSocket::wait_request_handler;  // 设置数据来时的读处理函数

    // 客户端应该主动发送第一次的数据，这里将读事件加入epoll监控
    if (epoll_add_event(s,                   // socket句柄
                        1,                   // 读
                        0,                   // 写
                        EPOLLET,             // 其它补充标记(EPOLLET(边缘触发)
                        EPOLL_CTL_ADD,       // 事件类型(增加/删除/修改)
                        newc) == -1){         // 连接池中的连接
      close_accepted_connection(newc);
    }
    break;  // 一般就执行一次
  }
}
// 用户连入，accept4时，得到的socket在处理中产生失败，则资源用这个函数释放
void CSocket::close_accepted_connection(lpconnection_t c) {
  // 回收连接池中连接，关闭socket
  int fd = c->fd;
  free_connection(c);
  c->fd = -1;     // 官方nginx这么写的，这么写有意义
  if (close(fd) == -1)
    loge("close_accepted_connection error.");
}
