//
// Created by zaiheshi on 1/27/23.
//
#include "xfly_socket.h"
#include "fmtlog.h"

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
  c->instance = !instance;   // 为0有效
  c->iCurrsequence = iCurrsequence;
  ++c->iCurrsequence;        // 取用时该值+1
  return c;
}
void CSocket::free_connection(lpconnection_t c) {
  c->data = m_pfree_connections;
  ++c->iCurrsequence; // 回收后该值也要+1(判断某些网络时间是否过期)
  m_pfree_connections = c;
  ++m_free_connection_n;
}
