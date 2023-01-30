//
// Created by zaiheshi on 1/27/23.
//
#include "xfly_socket.h"
#include "xfly_memory.h"
#include "fmtlog.h"
#include <arpa/inet.h>
// 来数据时候的处理，当连接上有数据来的时候，本函数会被epoll_process_events()所调用
void CSocket::wait_request_handler(lpconnection_t c) {
  // 假定客户端连入到服务器，要主动地给服务器先发送数据包
  ssize_t reco = recvproc(c, c->precvbuf, c->irecvlen);
  if (reco <= 0)
    return;
  if (c->curStat == _PKG_HD_INIT) {
    if (reco == m_iLenPkgHeader) {
      // 收到的数据只有包头(允许发生)
      wait_request_handler_proc_p1(c);
    }else {
      // 收到的包头不完整, 会不会一次性把包头和包体都接受到了? 不会，因为recvproc有个参数指定了返回值不过超过ireclen
      c->curStat = _PKG_HD_RECVING;
      c->precvbuf = c->precvbuf + reco;
      c->irecvlen = c->irecvlen - reco;
    }
  } else if (c->curStat == _PKG_HD_RECVING) {
    if (c->irecvlen == reco)        // 要收到的宽度和实际收到的宽度相等，表示包头收完整了，注意这里ireclen是包头长度
      wait_request_handler_proc_p1(c);
    else {
      // 还是没收完整
//      c->curStat = _PKG_HD_RECVING;
      c->precvbuf = c->precvbuf + reco;
      c->irecvlen = c->irecvlen - reco;
    }
  } else if (c->curStat == _PKG_BD_INIT) {
    // 包头刚好收完，准备接收包体
    if (reco == c->irecvlen) {
      // 包体收完整了
      wait_request_handler_proc_plast(c);
    } else {
      c->curStat = _PKG_BD_RECVING;
      c->precvbuf = c->precvbuf + reco;
      c->irecvlen = c->irecvlen - reco;
    }
  } else if (c->curStat == _PKG_BD_RECVING) {
    // 包体收完整了
    if (reco == c->irecvlen) {
      wait_request_handler_proc_plast(c);
    } else {
//      c->curStat = _PKG_BD_RECVING;
      c->precvbuf = c->precvbuf + reco;
      c->irecvlen = c->irecvlen - reco;
    }
  }
}

// 返回-1，则是有问题并且在这里把问题处理完毕了，调用本函数的调用者一般是可以直接return的
// 返回>0, 则是表示实际收到的字节数
ssize_t CSocket::recvproc(lpconnection_t c, char *buff, ssize_t bufflen) {
  // ssize_t是有符号整型，在32位机器上等同与int, 在64位机器上等同与long int, size_t就是无符号的ssize_t
  ssize_t n;
  n = recv(c->fd, buff, bufflen, 0);    // 系统函数，最后一个参数flag，一般为0
  if (n == 0) {
    // 客户端关闭(正常完成了4次挥手)
    close_connection(c);
    return -1;
  }
  // 有错误发生
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // LT 模式不应该出现这个errno
      return -1;
    }
    if (errno == EINTR) {
      // 信号中断导致的，不算错误
      return -1;
    }
    if (errno == ECONNRESET) {
      // 客户端拔电源
    }else {
      loge("recvproc error.");
    }
    close_connection(c);
    return -1;
  }
  return n;
}
void CSocket::wait_request_handler_proc_p1(lpconnection_t c) {
  // 包头收完整后的处理
  Memory* p_memory = Memory::getInstance();
  LPCOMM_PKG_HEADER pPkgHeader;
  // 包头数据已经在dataHeadInfo里面了
  pPkgHeader = (LPCOMM_PKG_HEADER)c->dataHeadInfo;

  unsigned short e_pkgLen;       // e_pkgLen是存储在包头结构中的包头+包体的总长度
  // 网络序转本机序，所有传输到网络上的2字节数据，都要用htons()转成网络序，所有从网络上收到的2字节数据，都要用ntohs转成本机序
  e_pkgLen = ntohs(pPkgHeader->pkgLen);
  // 恶意包或者错误包的判断, 什么时候需要用crc, 比如如何保证恶意数据不会干扰到正常数据包的接受?(属于不同的socket,不会被干扰)
  if (e_pkgLen < m_iLenPkgHeader) {
    // 复原初始状态, 因为其它状态可能调用这个函数
    c->curStat = _PKG_HD_INIT;
    c->precvbuf = c->dataHeadInfo;
    c->irecvlen = m_iLenPkgHeader;
  } else if (e_pkgLen > (_PKG_MAX_LENGTH-1000)) {
    // 复原初始状态, 因为其它状态可能调用这个函数
    c->curStat = _PKG_HD_INIT;
    c->precvbuf = c->dataHeadInfo;
    c->irecvlen = m_iLenPkgHeader;
  }else {
    // 分配内存开始收包体，因为包体长度并不是固定的，所以要分配内存[消息头长度+包头长度+包体长度]
    char* pTmpBuffer = (char*)p_memory->AllocMemory(m_iLenMsgHeader+e_pkgLen, false);
    c->ifnewrecvMem = true;             // 标记分配了内存
    c->pnewMemPointer = pTmpBuffer;     // 内存开始指针
    // 下面操作要小心，相当于把多个数据类型塞到char数组中
    // a. 填写消息头
    LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;  // 强制类型装换
    ptmpMsgHeader->pConn = c;
    ptmpMsgHeader->iCurrsequence = c->iCurrsequence;                    // 收到包时的连接池中连接序号记录到消息头，以备将来用
    // b. 填写包头
    pTmpBuffer += m_iLenMsgHeader;                                  // 跳过消息头，指向包头
    memcpy(pTmpBuffer, pPkgHeader, m_iLenPkgHeader);                   // 直接把收到的包头拷贝进来
    if (e_pkgLen == m_iLenPkgHeader) {
      // 允许一个包只有包头，没有包体
      wait_request_handler_proc_plast(c);                              // 放消息队列
    }else {
      // 开始收包体
      c->curStat = _PKG_BD_INIT;
      c->precvbuf = pTmpBuffer + m_iLenPkgHeader;                     // 指向待接收的包体位置
      c->irecvlen = e_pkgLen-m_iLenPkgHeader;                         // 剩余恰好是包体长度
    }
  }
}
void CSocket::wait_request_handler_proc_plast(lpconnection_t c) {
  // 把这段内存放到消息队列中
  inMsgRecvQueue(c->pnewMemPointer);
  // ...这里可能考虑出发业务逻辑，怎么触发业务逻辑，代码以后扩充
  c->ifnewrecvMem = false;            // 内存不需要释放，收好完整的包，这个包被移入消息队列，由业务逻辑去释放内存
  c->pnewMemPointer = nullptr;
  c->curStat = _PKG_HD_INIT;          // 状态机回到原始状态，为收下一个包做准备
  c->precvbuf = c->dataHeadInfo;
  c->irecvlen = m_iLenPkgHeader;
}
void CSocket::inMsgRecvQueue(char *buf) {
  m_MsgRecvQueue.push_back(buf);     // buf这段内存: 消息头+包头+包体
  // 日后需要增加释放内存的代码且逻辑处理应该引入多线程，所以这里要考虑临界问题

  tmpoutMsgRecvQueue();             // 临时调用，防止消息队列过大
  logi("非常好，受到了一个完整的数据包[包头+包体]!");
}
void CSocket::tmpoutMsgRecvQueue() {
  if (m_MsgRecvQueue.empty())
    return;
  int size = m_MsgRecvQueue.size();
  if (size < 1000)
    return;
  Memory* p_memory = Memory::getInstance();
  int cha = size-500;
  for (int i = 0; i < cha; ++i) {
    char* tmpMsgBuf = m_MsgRecvQueue.front();
    m_MsgRecvQueue.pop_front();
    p_memory->FreeMemory(tmpMsgBuf);
  }
}
