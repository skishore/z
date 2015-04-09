#include "gen/graph.h"

#include <algorithm>
#include <cfloat>
#include <map>
#include <set>

#include "base/debug.h"

using std::map;
using std::max;
using std::set;
using std::vector;

namespace babel {
namespace gen {

vector<Point> MinimumSpanningTree(const Graph& graph) {
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

Graph ComputeTreeDistances(const Graph& graph, const vector<Point>& tree) {
  const int n = graph.size();
  Graph result(n, vector<double>(n));
  if (n == 1) {
    return result;
  }

  map<int, vector<int>> adjacency;
  for (const Point& edge : tree) {
    adjacency[edge.x].push_back(edge.y);
    adjacency[edge.y].push_back(edge.x);
  }

  for (int i = 0; i < n; i++) {
    result[i][i] = 0;
    vector<int> frontier{i};
    set<int> visited{i};
    while (frontier.size() > 0) {
      const int node = frontier.back();
      frontier.pop_back();
      const double distance = result[i][node];
      for (const int child : adjacency.at(node)) {
        if (visited.find(child) == visited.end()) {
          result[i][child] = distance + graph[node][child];
          frontier.push_back(child);
          visited.insert(child);
        }
      }
    }
  }
  return result;
}

}  // namespace gen
}  // namespace babel
