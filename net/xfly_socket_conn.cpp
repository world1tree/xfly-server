//
// Created by zaiheshi on 1/27/23.
//
#include "fmtlog.h"
#include "xfly_memory.h"
#include "xfly_socket.h"

lpconnection_t CSocket::get_connection(int isock) {
  lpconnection_t c = m_pfree_connections;
  if (c == nullptr) {
    loge("get_connection return null.");
    return nullptr;
  }
  m_pfree_connections = c->data;  // 指向下一个
  m_free_connection_n--;

  // 保存一些变量，可能有用
  uintptr_t instance = c->instance;
  uint64_t iCurrsequence = c->iCurrsequence;

  memset(c, 0, sizeof(connection_t));
  c->fd = isock; // 具有唯一性
  c->curStat = _PKG_HD_INIT;
  c->precvbuf = c->dataHeadInfo;           // 很重要，指针最开始是指向这段内存的
  c->irecvlen = sizeof(COMM_PKG_HEADER);   // 先要求收包头这么长字节的数据
  c->ifnewrecvMem = false;                 // 标记没有new过内存
  c->pnewMemPointer = nullptr;

  c->instance = !instance;   // 为0有效
  c->iCurrsequence = iCurrsequence;
  ++c->iCurrsequence;        // 取用时该值+1
  return c;
}
void CSocket::free_connection(lpconnection_t c) {
  if (c->ifnewrecvMem) {
    Memory* p_memory = Memory::getInstance();
    p_memory->FreeMemory(c->pnewMemPointer);
    c->ifnewrecvMem = false;
    c->pnewMemPointer = nullptr;
  }
  c->data = m_pfree_connections;
  ++c->iCurrsequence; // 回收后该值也要+1(判断某些网络时间是否过期)
  m_pfree_connections = c;
  ++m_free_connection_n;
}
void CSocket::close_connection(lpconnection_t c) {
  // 回收连接池中连接，关闭socket
  if (close(c->fd) == -1)
    loge("close_accepted_connection error.");
  c->fd = -1;     // 官方nginx这么写的，这么写有意义
  free_connection(c);
}