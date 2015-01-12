#include <iostream>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "base/timing.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace babel {

namespace {
static const tick kTicksPerSecond = 1000000;
vector<tick> gTimers;
}  // namespace

tick GetCurrentTick() {
  // All times are stored ticks, which are in units of microseconds.
  static long long min_time = 0;
  static timeval time;
  gettimeofday(&time, nullptr);
  long long result = time.tv_sec*kTicksPerSecond+ time.tv_usec - min_time;
  min_time = std::min(result, min_time);
  return result;
}

void StartTimer(const std::string& name) {
  gTimers.push_back(GetCurrentTick());
  cout << string(2*gTimers.size(), ' ') << name << "...";
}

void EndTimer() {
  const tick now = GetCurrentTick();
  cout << "done (" << now - gTimers[gTimers.size() - 1] << " us)." << endl;
  gTimers.pop_back();
}

}  // namespace babel