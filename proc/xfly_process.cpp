//
// Created by zaiheshi on 1/24/23.
//
#include "xfly_func.h"
#include "xfly_global.h"

#include "fmtlog.h"
#include "xfly_conf.h"
#include "xfly_global.h"
#include <csignal>

static void startWorkProcess(int processNum);
static bool spawnProcess(int id);
[[noreturn]] static void workerProcessCycle(int id);

[[noreturn]] void masterProcessCycle() {
  processType = ProcessType::masterProcess;
  sigset_t set;
  sigemptyset(&set);
  // fork时防止信号干扰
  sigaddset(&set, SIGCHLD);  // 子进程状态改变
  sigaddset(&set, SIGALRM);  // 定时器超时
  sigaddset(&set, SIGIO);    // 异步I/O
  sigaddset(&set, SIGINT);   // 终端中断符
  sigaddset(&set, SIGHUP);   // 连接断开
  sigaddset(&set, SIGUSR1);  // 用户定义信号
  sigaddset(&set, SIGUSR2);  // 用户定义信号
  sigaddset(&set, SIGWINCH); // 终端窗口大小改变
  sigaddset(&set, SIGTERM);  // 终止
  sigaddset(&set, SIGQUIT);  // 终端退出符

  if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1)
    logw("sigprocmask失败.");
  logi("启动master进程, pid={}", getpid());
  fmtlog::poll(true);
  Config *conf = Config::getInstance();
  int workProcess = conf->getIntDefault("WorkerProcesses", 1);
  startWorkProcess(workProcess);
  sigemptyset(&set); // 清空信号集合，表示不屏蔽任何信号
  for (;;) {
//    logi("这是父进程, pid={}.", getpid());
//    fmtlog::poll();
    sigsuspend(&set); // 进程处于挂起状态，完全依靠信号驱动, 原子操作，不允许信号嵌套，一旦来了某个信号，会阻塞之前定义的10个信号
//    logi("执行到sigsuspend下面.");
//    fmtlog::poll();
//    sleep(1);
  }
}

static void startWorkProcess(int processNum) {
  for (int i = 0; i < processNum; ++i)
    spawnProcess(i);
}

static bool spawnProcess(int id) {
  pid_t pid;
  pid = fork();
  switch (pid) {
  case -1:
    loge("spawnProcess失败，id={}", id);
    return false;
  case 0:
    logi("启动worker进程, pid={}", getpid());
    fmtlog::poll(true);
    workerProcessCycle(id);
    break;
  default:
    break;
  }
  return true;
}

[[noreturn]] static void workerProcessCycle(int id) {
  sigset_t set;
  sigemptyset(&set);
  processType = ProcessType::workerprocess;
  // 恢复这个子进程的信号集, 该子进程可以正常接收信号
  if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    logw("sigprocmask失败.");
  for (;;) {
//    logi("这是子进程, pid={}.", getpid());
//    fmtlog::poll();
    sleep(1);
  }
}