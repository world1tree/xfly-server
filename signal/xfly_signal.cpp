//
// Created by zaiheshi on 1/24/23.
//
#include "fmtlog.h"
#include "xfly_func.h"
#include "xfly_global.h"
#include "signal.h"
#include <cstring>
#include <sys/wait.h>

typedef struct {
  int signo;
  const char *signame;
  void (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
} xfly_signal_t;

static void signalHandler(int signo, siginfo_t *siginfo, void *ucontext);
static void cleanWorker();

xfly_signal_t signals[] = {
    // signo      signame             handler
    {SIGHUP, "SIGHUP",
     signalHandler}, // 终端断开信号，对于守护进程常用于reload重载配置文件通知--标识1
    {SIGINT, "SIGINT", signalHandler},   // 标识2
    {SIGTERM, "SIGTERM", signalHandler}, // 标识15
    {SIGCHLD, "SIGCHLD",
     signalHandler}, // 子进程退出时，父进程会收到这个信号--标识17
    {SIGQUIT, "SIGQUIT", signalHandler}, // 标识3
    {SIGIO, "SIGIO", signalHandler}, // 指示一个异步I/O事件【通用异步I/O信号】
    {SIGSYS, "SIGSYS, SIG_IGN",
     nullptr}, // 我们想忽略这个信号，SIGSYS表示收到了一个无效系统调用，如果我们不忽略，进程会被操作系统杀死，--标识31
    {0, nullptr, nullptr} // 信号对应的数字至少是1，所以可以用0作为一个特殊标记
};

bool initSignals() {
  xfly_signal_t *sig;
  struct sigaction sa {}; // 系统定义的和信号有关的一个结构
  for (sig = signals; sig->signo != 0; ++sig) {
    memset(&sa, 0, sizeof(struct sigaction));
    if (sig->handler) {
      sa.sa_sigaction = sig->handler; // 指定信号处理函数
      sa.sa_flags =
          SA_SIGINFO; // 信号附带的参数可以被传递到信号处理函数中，也就是想让sa_sigaction指定的信号处理函数生效，必须把sa_flags设定为SA_SIGINFO
    } else {
      sa.sa_handler = SIG_IGN;
    }
    sigemptyset(&sa.sa_mask);
    if (sigaction(sig->signo, &sa, nullptr) == -1) {
      loge("sigaction[{}] failed. ", sig->signo);
      return false;
    }
  }
  return true;
}

void signalHandler(int signo, siginfo_t *siginfo, void *ucontext) {
  xfly_signal_t *sig;
  //  char* action;
  for (sig = signals; sig->signo != 0; ++sig) {
    if (sig->signo == signo)
      break;
  }

  if (processType == ProcessType::masterProcess) {
    switch (signo) {
    case SIGCHLD:
      invalidProcess = 1;
      break;
    default:
      break;
    }
  } else if (processType == ProcessType::workerprocess) {

  } else {

  }

  // 打印日志
  if (siginfo && siginfo->si_pid) {
    logi("signal {} received from pid: {}.", sig->signame, siginfo->si_pid);
  } else {
    logi("signal {} received.", sig->signame);
  }
  if (signo == SIGCHLD) {
    // 由master释放worker的资源
    cleanWorker();
  }
  fmtlog::poll(true);
}

static void cleanWorker() {
  pid_t pid;
  int status;
  int err;
  bool success = false;

  for (;;) {
    // 正常流程, 第一次调用返回值>0, 第二次调用返回值=0
    pid =waitpid(-1, &status, WNOHANG);
    if (pid == 0) {
      return;
    } else if (pid == -1) {
      err = errno;
      if (err == EINTR) // 调用被某个信号中断
          continue;
      if (err == ECHILD && success) // 没有子进程
          return;
      if (err == ECHILD) { // 没有子进程
          logi("waitpid failed!");
          return;
      }
      // 其它情况
      logi("waitpid failed!");
      return;
    }
    success = true;
    if (WTERMSIG(status)) {   //  获取使子进程中止的进程编号
      logi("pid = {} exited on signal {}!", pid, WTERMSIG(status));
    } else {                 // 获取子进程传递给exit或_exit参数的低8位
      logi("pid = {} exited with code {}!", pid, WEXITSTATUS(status));
    }
  }
}