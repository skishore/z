#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"

namespace skishore {
namespace {

const char* kBinary;

int addr2line(const char* binary, const void* address) {
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

void SignalHandler(int signal, siginfo_t* info, void* secret) {
  static const int kTracebackSize = 64; 
  void* stack_trace[kTracebackSize];
  size_t size = backtrace(stack_trace, kTracebackSize);

  ucontext_t* context = (ucontext_t*)secret;
  fprintf(stderr, "Error: caught signal %d.\n", signal);
  if (signal == SIGSEGV) {
    //printf("Details: tried to dereference %p from %p.\n",
    //       info->si_addr, context->uc_mcontext->gregs[REG_EIP]);
  }

  backtrace_symbols(stack_trace, size); 
  for (int i = 0; i < size - 1; i++) {
    addr2line(kBinary, stack_trace[i]);
  }
  exit(1);
}

} // namespace


void RegisterCrashHandlers(const char* binary) {
  kBinary = binary;
  struct sigaction action;
  action.sa_sigaction = SignalHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(SIGSEGV, &action, nullptr);
  sigaction(SIGUSR1, &action, nullptr);
}

}  // namespace skishore
