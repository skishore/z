#include "gen/graph.h"

#include <algorithm>
#include <cfloat>
#include <map>

#include "base/debug.h"

using std::map;
using std::max;
using std::vector;

namespace babel {
namespace gen {

vector<Point> MinimumSpanningTree(const vector<vector<double>>& graph) {
  vector<Point> result;
  map<int, double> distances;
  map<int, int> parents;

  const int n = graph.size();
  for (int i = 1; i < n; i++) {
    distances[i] = graph[0][i];
    parents[i] = 0;
  }

  while (distances.size() > 0) {
    int best_index = -1;
    double best_distance = DBL_MAX;
    for (const auto& pair : distances) {
      if (pair.second <= best_distance) {
        best_index = pair.first;
        best_distance = pair.second;
      }
    }
    result.push_back({best_index, parents.at(best_index)});
    distances.erase(best_index);
    parents.erase(best_index);
    for (auto& pair : distances) {
      const int i = pair.first;
      if (graph[best_index][i] < pair.second) {
        pair.second = graph[best_index][i];
        parents[i] = best_index;
      }
    }
  }

  return result;
}

}  // namespace gen
}  // namespace babel
