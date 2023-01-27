//
// Created by zaiheshi on 1/27/23.
//
#include "xfly_socket.h"
#include "fmtlog.h"
#include <arpa/inet.h>
#include <stdio.h>
size_t CSocket::sock_ntop(struct sockaddr *sa, int port, u_char *text, size_t len) {
  loge("calling sock_ntop, not implement yet.");
  exit(2);
//  struct sockaddr_in* sin;
//  u_char *p;
//  switch (sa->sa_family) {
//  case AF_INET:
//    sin = (struct sockaddr_in*)sa;
//    p = (u_char*)&sin->sin_addr;
//    if (port) {
//      sprintf(text, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
//    }
}
