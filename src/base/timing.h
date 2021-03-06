#ifndef __BABEL_BASE_TIMING_H__
#define __BABEL_BASE_TIMING_H__

#include <string>

namespace babel {

typedef long long tick;

tick GetCurrentTick();

// All calls to StartTimer have to be followed by a call to EndTimer.
void StartTimer(const std::string& name);
void EndTimer();

void SetTimerVerbosity(bool verbose);

}  // namespace babel

#endif  // __BABEL_BASE_TIMING_H__
