#include <algorithm>
#include <map>

#include "debug.h"
#include "util.h"
#include "Log.h"

using std::map;
using std::max;
using std::string;
using std::vector;

namespace babel {

namespace {
static const int kMaxLogSize = 24;
}

void Log::AddLine(const string& line) {
  new_lines_.push_back(line);
}

void Log::Coalesce() {
  fresh_ = !new_lines_.empty();
  if (!fresh_) {
    return;
  }

  map<string,int> counts;
  for (const string& new_line : new_lines_) {
    counts[new_line] += 1;
  }

  string line;
  for (const string& new_line : new_lines_) {
    const int count = counts[new_line];
    if (count > 0) {
      line += (line.empty() ? "" : " ") + new_line;
      if (count > 1) {
        line += " (x" + IntToString(count) + ")";
      }
      counts[new_line] = 0;
    }
  }
  new_lines_.clear();
  lines_.push_back(line);
  if (lines_.size() > kMaxLogSize) {
    lines_.pop_front();
  }
}

bool Log::IsFresh() const {
  return fresh_;
}

vector<string> Log::GetLastLines(int n) const {
  vector<string> result;
  const int index = max((int)(lines_.size() - n), 0);
  for (int i = index; i < lines_.size(); i++) {
    result.push_back(lines_[i]);
  }
  return result;
}

}  // namespace babel
