//
// Created by zaiheshi on 1/28/23.
//

#ifndef XFLY_XFLY_COMM_H
#define XFLY_XFLY_COMM_H

#define _PKG_MAX_LENGTH 30000   // 包头+包体不超过这个长度，实际比这小
#define _PKG_HD_INIT 0          // 初始状态， 准备接收数据包头
#define _PKG_HD_RECVING 1       // 接收数据包头, 包头不完整，继续接收中
#define _PKG_BD_INIT 2          // 包头接收完，准备接收包体
#define _PKG_BD_RECVING 3       // 接收包体, 包体不完整，继续接收中, 结束后回到_PKG_HD_INIT

#define _DATA_BUFSIZE_ 20       // 固定大小数组接受包头，需>sizeof(COMM_PKG_HEADER)

#pragma pack (1)               // 1字节对齐

typedef struct {
  unsigned short pkgLen;         // 包头+包体，2字节，最大数字6万多，超过_PKG_MAX_LENGTH
  unsigned short msgCode;        // 消息类型代码，2字节，区别每个不同的消息
  int crc32;                     // 校验码
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

#pragma pack()

#endif // XFLY_XFLY_COMM_H
