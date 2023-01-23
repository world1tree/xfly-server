//
// Created by zaiheshi on 1/20/23.
//
#include <cstdlib>
#include <stdio.h>

#include "fmtlog.h"
#include "xfly_conf.h"

void init_config();
void init_log();

int main() {
  init_config();
  init_log();

  return 0;
}

void init_config() { // TODO: hard-code
  // 配置文件路径
  const char* configPath = "/home/zaiheshi/CodeReps/xfly/xfly.conf";
  auto conf = Config::getInstance();
  if (!conf->load(configPath)) {
    printf("load config %s failed.", configPath);
    exit(1);
  }
}

void init_log() {
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
  fmtlog::setHeaderPattern("{HMS} {s:<16} {l}[{t:<6}] ");
}
