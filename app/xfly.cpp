//
// Created by zaiheshi on 1/20/23.
//
#include <cstdlib>
#include <stdio.h>

#include "signal.h"
#include "fmtlog.h"
#include "xfly_conf.h"
#include "xfly_func.h"
#include "xfly_socket.h"
//#include "xfly_global.h"

void initLog();
void freeSource();
ProcessType processType; // 标记进程类型
sig_atomic_t invalidProcess;
CSocket g_socket;

int main() {
  // 读取配置文件
  const char* configPath = "/home/zaiheshi/CodeReps/xfly/xfly.conf";
  auto conf = Config::getInstance();
  if (!conf->load(configPath)) {
    printf("load config %s failed.", configPath);
    exit(1);
  }
  // 初始化日志
  initLog();
  // 初始化信号处理函数
  if (!initSignals())
    exit(1);
  // 初始化socket
  if (!g_socket.Initialize())
    exit(1);
  // 设置守护进程
  int daemon = conf->getIntDefault("Daemon", 0);
  if (daemon == 1) {
    int processId = createDaemon();
    if (processId == 1 || processId == -1) {
      exit(1); // 父进程直接退出, 失败直接退出
    }
  }
  // 主进程循环
  masterProcessCycle();
  freeSource();
  return 0;
}

void initLog() {
  auto conf = Config::getInstance();
  fmtlog::LogLevel toLevel[] = {
      fmtlog::DBG,
      fmtlog::INF,
      fmtlog::WRN,
      fmtlog::ERR,
      fmtlog::OFF
  };
  const char* logFile = conf->getString("Log");
  int level = conf->getIntDefault("LogLevel", 1);
  fmtlog::setLogFile(logFile);
  fmtlog::setLogLevel(toLevel[level]);
  fmtlog::setHeaderPattern("{HMS} {s:<16} {l} ");
}

void freeSource() {
  fmtlog::closeLogFile();
}