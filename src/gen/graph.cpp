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
  map<int, double> distance;
  map<int, int> parent;

  const int n = graph.size();
  for (int i = 1; i < n; i++) {
    distance[i] = graph[0][i];
    parent[i] = 0;
  }

  while (distance.size() > 0) {
    int best_index = -1;
    double best_distance = DBL_MAX;
    for (const auto& pair : distance) {
      if (pair.second <= best_distance) {
        best_index = pair.first;
        best_distance = pair.second;
      }
    }
    result.push_back({best_index, parent.at(best_index)});
    distance.erase(best_index);
    parent.erase(best_index);
    for (auto& pair : distance) {
      const int i = pair.first;
      if (graph[best_index][i] < pair.second) {
        pair.second = graph[best_index][i];
        parent[i] = best_index;
      }
    }
  }

  return result;
}

}  // namespace gen
}  // namespace babel
