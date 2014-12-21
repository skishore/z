#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"

namespace skishore {
namespace {

const char* kBinary;

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

inline void SignalHandler(int signal) {
  static const int kTracebackSize = 64;
  void* stack_trace[kTracebackSize];
  size_t size = backtrace(stack_trace, kTracebackSize);
  fprintf(stderr, "Error: caught signal %d:\n", signal);
  backtrace_symbols_fd(stack_trace, size, STDERR_FILENO);
  for (int i = 3; i < size - 1; i++) {
    addr2line(kBinary, stack_trace[i]);
  }
  exit(1);
}

} // namespace


void RegisterCrashHandlers(const char* binary) {
  kBinary = binary;
  signal(SIGSEGV, SignalHandler);
}

}  // namespace skishore
