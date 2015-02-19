#include "debug.h"

#ifndef EMSCRIPTEN

#include <execinfo.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace {

static const char* kBinary;
static const bool kUseLLDB = true;

inline int addr2line(const char* binary, const void* address) {
  char command[1024] = {0};
  #ifdef __APPLE__
    sprintf(command, "atos -o %.256s %p", binary, address);
  #else
    sprintf(command, "addr2line -f -p -e %.256s %p", binary, address);
  #endif
  // Calling the addr2line utility (atos on mac) will print function::file::line
  // information for the given address in the binary directly to stdout.
  return system(command);
}

void AttachLLDBToCurrentProcess() {
  char pid[64];
  sprintf(pid, "%d", getpid());
  int child_pid = fork();
  if (!child_pid) {
    fprintf(stdout, "Stack trace for pid %s:\n", pid);
    execlp("lldb", "lldb", "-attach-pid", pid, "--one-line", "bt", nullptr);
  } else {
    waitpid(child_pid, nullptr, 0);
  }
}

inline void SignalHandler(int signal) {
  static const int kTracebackSize = 64;
  void* stack_trace[kTracebackSize];
  size_t size = backtrace(stack_trace, kTracebackSize);
  fprintf(stderr, "Error: caught signal %d:\n", signal);
  backtrace_symbols_fd(stack_trace, size, STDERR_FILENO);
  if (kUseLLDB) {
    AttachLLDBToCurrentProcess();
  } else {
    for (int i = 3; i < size - 1; i++) {
      addr2line(kBinary, stack_trace[i]);
    }
  }
  exit(1);
}

}  // namespace

#endif  // EMSCRIPTEN

namespace babel {

void RegisterCrashHandlers(const char* binary) {
  #ifndef EMSCRIPTEN
  kBinary = binary;
  signal(SIGSEGV, SignalHandler);
  signal(SIGFPE, SignalHandler);
  #endif  // EMSCRIPTEN
}

}  // namespace babel
