//
// Created by zaiheshi on 1/24/23.
//

#include "fmtlog.h"
#include "signal.h"
#include "xfly_func.h"
#include <fcntl.h>
#include <sys/stat.h>

int createDaemon() {
  switch(fork()) {
  case -1:
    loge("createDaemon failed.");
    fmtlog::poll();
    return -1;
  case 0:
    // 子进程，继续往下走
    break;
  default:
    // 父进程返回1, 需要退出exit
    return 1;
  }
  // 子进程作为守护进程，承担master进程的责任, 重定向标准输入/输出
  if (setsid() == -1) {
    loge("setsid failed.");
    fmtlog::poll();
    return -1;
  }
  umask(0);
  int fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    loge("open /dev/null failed.");
    fmtlog::poll();
    return -1;
  }
  if (dup2(fd, STDIN_FILENO) == -1) {
    loge("dup2(STDIN) failed.");
    fmtlog::poll();
    return -1;
  }

  if (dup2(fd, STDOUT_FILENO) == -1) {
    loge("dup2(STDOUT) failed.");
    fmtlog::poll();
    return -1;
  }

  if (fd > STDERR_FILENO) {
    if (close(fd) == -1) {
      loge("close(fd) failed.");
      fmtlog::poll();
      return -1;
    }
  }
  return 0;
}
