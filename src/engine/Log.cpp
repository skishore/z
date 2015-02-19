#include "engine/Log.h"

#include <algorithm>
#include <map>

#include "base/debug.h"
#include "base/util.h"

using std::map;
using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {

namespace {
static const int kMaxLogSize = 24;
}

void Log::AddLine(const string& line) {
  new_lines_.push_back(line);
}

void Log::Flush(bool changed) {
  open_ = false;
  if (!changed) {
    new_lines_.clear();
    return;
  }

  fresh_ = !new_lines_.empty();
  if (!fresh_) {
    return;
  }

  lines_.push_back(GetCurrentLine());
  new_lines_.clear();
  if (lines_.size() > kMaxLogSize) {
    lines_.pop_front();
  }
}

void Log::Open() {
  open_ = true;
}

string Log::GetCurrentLine() const {
  string line;
  for (const string& new_line : new_lines_) {
    line += (line.empty() ? "" : " ") + new_line;
  }
  return line;
}

vector<string> Log::GetLastLines(int n) const {
  vector<string> result;
  if (open_ && !new_lines_.empty()) {
    result.push_back(GetCurrentLine());
    n -= 1;
  }
  const int index = max((int)(lines_.size() - n), 0);
  for (int i = index; i < lines_.size(); i++) {
    result.push_back(lines_[i]);
  }
  return result;
}

bool Log::IsFresh() const {
  return (open_ && !new_lines_.empty()) || (!open_ && fresh_);
}

}  // namespace engine
}  // namespace babel
