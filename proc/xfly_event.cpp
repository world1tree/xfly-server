//
// Created by zaiheshi on 1/27/23.
//
#include "xfly_func.h"
#include "xfly_global.h"

void xfly_process_events_and_timer() {
  g_socket.epoll_process_event(-1);   // -1表示卡着等待
}
