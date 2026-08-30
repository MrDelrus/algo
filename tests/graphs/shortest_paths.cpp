// Correctness tests for algo::graphs: dijkstra, bellman_ford and get_components.
//
// The reference for distances is Floyd-Warshall on a dense matrix — a different algorithm, not a
// variation of the one under test. The reference for components is a transitive closure of
// reachability, computed the same obvious way.
//
// Deterministic throughout: graphs come from a counter-based splitmix64 with a fixed seed.

#include "algo_library.hpp"

#include "../harness.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

using testing::check;
using testing::check_equal;

constexpr std::uint64_t seed = 0xa5a5a5a5deadbeefULL;
constexpr std::int64_t none = gr::unreachable;
constexpr std::int64_t below = gr::unbounded_negative;

std::string at(std::size_t size, std::size_t from, std::size_t to) {
  return "n=" + std::to_string(size) + " " + std::to_string(from) + "->" + std::to_string(to);
}

// All-pairs shortest paths, the obvious way.
std::vector<std::vector<std::int64_t>> floyd_warshall(const gr::graph_weighted& edges) {
  std::size_t size = edges.size();
  std::vector<std::vector<std::int64_t>> best(size, std::vector<std::int64_t>(size, none));
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    best[vertex][vertex] = 0;
  }
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    for (const auto& [destination, weight] : edges[vertex]) {
      std::size_t target = static_cast<std::size_t>(destination);
      best[vertex][target] = std::min(best[vertex][target], weight);
    }
  }
  for (std::size_t middle = 0; middle < size; ++middle) {
    for (std::size_t from = 0; from < size; ++from) {
      if (best[from][middle] == none) {
        continue;
      }
      for (std::size_t to = 0; to < size; ++to) {
        if (best[middle][to] == none) {
          continue;
        }
        std::int64_t candidate = best[from][middle] + best[middle][to];
        if (candidate < best[from][to]) {
          best[from][to] = candidate;
        }
      }
    }
  }
  return best;
}

// Reachability closure, used as the reference for components.
std::vector<std::vector<bool>> reachability(const gr::graph& edges) {
  std::size_t size = edges.size();
  std::vector<std::vector<bool>> reaches(size, std::vector<bool>(size, false));
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    reaches[vertex][vertex] = true;
    for (std::int64_t destination : edges[vertex]) {
      reaches[vertex][static_cast<std::size_t>(destination)] = true;
    }
  }
  for (std::size_t middle = 0; middle < size; ++middle) {
    for (std::size_t from = 0; from < size; ++from) {
      if (!reaches[from][middle]) {
        continue;
      }
      for (std::size_t to = 0; to < size; ++to) {
        if (reaches[middle][to]) {
          reaches[from][to] = true;
        }
      }
    }
  }
  return reaches;
}

gr::graph_weighted random_weighted(testing::random_source& source, std::size_t size,
                                   std::int64_t weight_bound, std::size_t density) {
  gr::graph_weighted edges(size);
  for (std::size_t from = 0; from < size; ++from) {
    for (std::size_t to = 0; to < size; ++to) {
      if (from != to && source.below(10) < density) {
        edges[from].emplace_back(
            static_cast<std::int64_t>(to),
            static_cast<std::int64_t>(source.below(static_cast<std::size_t>(weight_bound))));
      }
    }
  }
  return edges;
}

// Undirected: every edge appears in both lists, which is what get_components assumes.
gr::graph random_undirected(testing::random_source& source, std::size_t size, std::size_t density) {
  gr::graph edges(size);
  for (std::size_t from = 0; from < size; ++from) {
    for (std::size_t to = from + 1; to < size; ++to) {
      if (source.below(10) < density) {
        edges[from].push_back(static_cast<std::int64_t>(to));
        edges[to].push_back(static_cast<std::int64_t>(from));
      }
    }
  }
  return edges;
}

void dijkstra_against_floyd_warshall() {
  testing::section("dijkstra against floyd-warshall");
  testing::random_source source(seed);

  for (std::size_t size = 1; size <= 8; ++size) {
    for (std::size_t density : {std::size_t(1), std::size_t(4), std::size_t(9)}) {
      for (std::size_t attempt = 0; attempt < 6; ++attempt) {
        gr::graph_weighted edges = random_weighted(source, size, 50, density);
        std::vector<std::vector<std::int64_t>> expected = floyd_warshall(edges);

        for (std::size_t start = 0; start < size; ++start) {
          std::vector<std::int64_t> distance =
              gr::dijkstra(edges, static_cast<std::int64_t>(start));
          check_equal(distance.size(), size, "one distance per vertex");
          for (std::size_t finish = 0; finish < size; ++finish) {
            check_equal(distance[finish], expected[start][finish],
                        "dijkstra " + at(size, start, finish));
            check_equal(gr::dijkstra(edges, static_cast<std::int64_t>(start),
                                     static_cast<std::int64_t>(finish)),
                        expected[start][finish],
                        "dijkstra with an early exit " + at(size, start, finish));
          }
        }
      }
    }
  }
}

// Negative weights without a negative cycle, built from potentials: an edge from u to v gets
// weight base + p[u] - p[v] with base >= 0, which cannot close a negative cycle because the
// potentials cancel around any loop.
gr::graph_weighted random_without_negative_cycles(testing::random_source& source,
                                                  std::size_t size) {
  std::vector<std::int64_t> potential(size);
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    potential[vertex] = static_cast<std::int64_t>(source.below(40)) - 20;
  }
  gr::graph_weighted edges(size);
  for (std::size_t from = 0; from < size; ++from) {
    for (std::size_t to = 0; to < size; ++to) {
      if (from != to && source.below(10) < 4) {
        std::int64_t base = static_cast<std::int64_t>(source.below(10));
        edges[from].emplace_back(static_cast<std::int64_t>(to),
                                 base + potential[from] - potential[to]);
      }
    }
  }
  return edges;
}

void bellman_ford_against_floyd_warshall() {
  testing::section("bellman-ford against floyd-warshall");
  testing::random_source source(seed ^ 0x1111111111111111ULL);

  for (std::size_t size = 1; size <= 8; ++size) {
    for (std::size_t attempt = 0; attempt < 12; ++attempt) {
      gr::graph_weighted edges = random_without_negative_cycles(source, size);
      std::vector<std::vector<std::int64_t>> expected = floyd_warshall(edges);

      for (std::size_t start = 0; start < size; ++start) {
        std::vector<std::int64_t> distance =
            gr::bellman_ford(edges, static_cast<std::int64_t>(start));
        for (std::size_t finish = 0; finish < size; ++finish) {
          check(distance[finish] != below, "nothing is unbounded where no negative cycle exists");
          check_equal(distance[finish], expected[start][finish],
                      "bellman-ford " + at(size, start, finish));
          check_equal(gr::bellman_ford(edges, static_cast<std::int64_t>(start),
                                       static_cast<std::int64_t>(finish)),
                      expected[start][finish], "bellman-ford to one " + at(size, start, finish));
        }
      }
    }
  }
}

// With a negative cycle in reach, a shortest path stops existing for every vertex the cycle can
// reach. The reference works that out from Floyd-Warshall: a vertex sits on a negative cycle
// when best[v][v] < 0, and the affected set is everything such a vertex reaches, provided the
// start reaches it in the first place.
std::vector<std::int64_t> classify(const gr::graph_weighted& edges, std::size_t start) {
  std::size_t size = edges.size();
  std::vector<std::vector<std::int64_t>> best = floyd_warshall(edges);
  gr::graph plain(size);
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    for (const auto& [destination, weight] : edges[vertex]) {
      static_cast<void>(weight);
      plain[vertex].push_back(destination);
    }
  }
  std::vector<std::vector<bool>> reaches = reachability(plain);

  std::vector<std::int64_t> expected(size, none);
  for (std::size_t finish = 0; finish < size; ++finish) {
    if (!reaches[start][finish]) {
      continue;
    }
    bool affected = false;
    for (std::size_t cycle = 0; cycle < size; ++cycle) {
      if (best[cycle][cycle] < 0 && reaches[start][cycle] && reaches[cycle][finish]) {
        affected = true;
      }
    }
    expected[finish] = affected ? below : best[start][finish];
  }
  return expected;
}

void negative_cycles_against_the_classification() {
  testing::section("negative cycles against the classification");
  testing::random_source source(seed ^ 0x5555555555555555ULL);

  for (std::size_t size = 1; size <= 7; ++size) {
    for (std::size_t attempt = 0; attempt < 40; ++attempt) {
      // Unconstrained negative weights, so negative cycles appear on their own.
      gr::graph_weighted edges(size);
      for (std::size_t from = 0; from < size; ++from) {
        for (std::size_t to = 0; to < size; ++to) {
          if (source.below(10) < 3) {
            edges[from].emplace_back(static_cast<std::int64_t>(to),
                                     static_cast<std::int64_t>(source.below(11)) - 5);
          }
        }
      }

      for (std::size_t start = 0; start < size; ++start) {
        std::vector<std::int64_t> distance =
            gr::bellman_ford(edges, static_cast<std::int64_t>(start));
        std::vector<std::int64_t> expected = classify(edges, start);
        for (std::size_t finish = 0; finish < size; ++finish) {
          check_equal(distance[finish], expected[finish],
                      "bellman-ford classifies " + at(size, start, finish));
          check_equal(gr::bellman_ford(edges, static_cast<std::int64_t>(start),
                                       static_cast<std::int64_t>(finish)),
                      expected[finish], "and its single-distance form " + at(size, start, finish));
        }
      }
    }
  }
}

void negative_cycles_by_hand() {
  testing::section("negative cycles by hand");

  {
    // 0 -> 1 -> cycle{2,3,4}, and 5 hangs off the cycle. 6 reaches the start but is not reached.
    gr::graph_weighted edges(7);
    edges[0] = {{1, 10}};
    edges[1] = {{2, 1}};
    edges[2] = {{3, -1}};
    edges[3] = {{4, -1}};
    edges[4] = {{2, -1}, {5, 7}};
    edges[6] = {{0, 1}};
    std::vector<std::int64_t> distance = gr::bellman_ford(edges, 0);
    check_equal(distance[0], std::int64_t(0), "the start before the cycle keeps its distance");
    check_equal(distance[1], std::int64_t(10), "and so does the vertex before it");
    check_equal(distance[2], below, "a vertex on the cycle is unbounded");
    check_equal(distance[4], below, "all of it");
    check_equal(distance[5], below, "and what hangs off it");
    check_equal(distance[6], none, "a vertex the start cannot reach stays unreachable");
  }
  {
    // The cycle exists but nothing from the start reaches it.
    gr::graph_weighted edges(5);
    edges[0] = {{1, 7}};
    edges[2] = {{3, -1}};
    edges[3] = {{4, -1}};
    edges[4] = {{2, -1}};
    std::vector<std::int64_t> distance = gr::bellman_ford(edges, 0);
    check_equal(distance[0], std::int64_t(0), "an unreachable cycle changes nothing");
    check_equal(distance[1], std::int64_t(7), "distances stay finite");
    check_equal(distance[2], none, "and its vertices stay unreachable");
  }
  {
    // A negative self-loop is a negative cycle of length one.
    gr::graph_weighted edges(2);
    edges[0] = {{0, -1}, {1, 5}};
    std::vector<std::int64_t> distance = gr::bellman_ford(edges, 0);
    check_equal(distance[0], below, "a negative self-loop unbounds its own vertex");
    check_equal(distance[1], below, "and everything after it");
  }
  {
    // Zero is not negative.
    gr::graph_weighted edges(3);
    edges[0] = {{1, 1}};
    edges[1] = {{2, 0}};
    edges[2] = {{1, 0}};
    std::vector<std::int64_t> distance = gr::bellman_ford(edges, 0);
    check_equal(distance[1], std::int64_t(1), "a zero-weight cycle leaves distances finite");
    check_equal(distance[2], std::int64_t(1), "throughout");
  }
  {
    // A single negative edge, no cycle at all.
    gr::graph_weighted edges(3);
    edges[0] = {{1, 5}};
    edges[1] = {{2, -3}};
    std::vector<std::int64_t> distance = gr::bellman_ford(edges, 0);
    check_equal(distance[2], std::int64_t(2), "negative weights alone are fine");
  }
}

void unweighted_graphs_count_edges() {
  testing::section("unweighted graphs count edges");
  testing::random_source source(seed ^ 0x2222222222222222ULL);

  for (std::size_t size = 1; size <= 8; ++size) {
    for (std::size_t attempt = 0; attempt < 8; ++attempt) {
      gr::graph plain = random_undirected(source, size, 4);

      // The same graph with every weight one, for the reference.
      gr::graph_weighted weighted(size);
      for (std::size_t vertex = 0; vertex < size; ++vertex) {
        for (std::int64_t destination : plain[vertex]) {
          weighted[vertex].emplace_back(destination, 1);
        }
      }
      std::vector<std::vector<std::int64_t>> expected = floyd_warshall(weighted);

      for (std::size_t start = 0; start < size; ++start) {
        std::vector<std::int64_t> distance = gr::dijkstra(plain, static_cast<std::int64_t>(start));
        std::vector<std::int64_t> alternative =
            gr::bellman_ford(plain, static_cast<std::int64_t>(start));
        for (std::size_t finish = 0; finish < size; ++finish) {
          check_equal(distance[finish], expected[start][finish],
                      "dijkstra on an unweighted graph " + at(size, start, finish));
          check_equal(alternative[finish], expected[start][finish],
                      "bellman-ford on an unweighted graph " + at(size, start, finish));
        }
      }
    }
  }
}

void degenerate_inputs() {
  testing::section("degenerate inputs");

  {
    gr::graph_weighted single(1);
    check_equal(gr::dijkstra(single, 0).size(), std::size_t(1), "one vertex, one distance");
    check_equal(gr::dijkstra(single, 0)[0], std::int64_t(0), "and it is zero");
    check_equal(gr::dijkstra(single, 0, 0), std::int64_t(0), "start equals finish");
    check_equal(gr::bellman_ford(single, 0)[0], std::int64_t(0), "bellman-ford agrees");
  }
  {
    // Self-loops and parallel edges must not confuse either algorithm.
    gr::graph_weighted edges(3);
    edges[0] = {{0, 5}, {1, 10}, {1, 3}, {1, 7}};
    edges[1] = {{2, 1}, {2, 100}};
    std::vector<std::int64_t> distance = gr::dijkstra(edges, 0);
    check_equal(distance[1], std::int64_t(3), "the cheapest parallel edge wins");
    check_equal(distance[2], std::int64_t(4), "and the path through it");
    check_equal(gr::bellman_ford(edges, 0)[2], std::int64_t(4), "bellman-ford agrees");
  }
  {
    // Zero weights are allowed and must not be mistaken for absent edges.
    gr::graph_weighted edges(3);
    edges[0] = {{1, 0}};
    edges[1] = {{2, 0}};
    std::vector<std::int64_t> distance = gr::dijkstra(edges, 0);
    check_equal(distance[1], std::int64_t(0), "a zero-weight edge is still an edge");
    check_equal(distance[2], std::int64_t(0), "and so is a path of them");
    check(distance[2] != none, "which is not the same as unreachable");
  }
  {
    gr::graph_weighted disconnected(4);
    disconnected[0] = {{1, 2}};
    disconnected[2] = {{3, 2}};
    std::vector<std::int64_t> distance = gr::dijkstra(disconnected, 0);
    check_equal(distance[2], none, "an unreachable vertex reports unreachable");
    check_equal(gr::dijkstra(disconnected, 0, 3), none, "and so does the single-distance form");
    check_equal(gr::bellman_ford(disconnected, 0)[3], none, "bellman-ford agrees");
  }
  {
    // Starting anywhere but zero must work the same.
    gr::graph_weighted edges(4);
    edges[3] = {{0, 1}};
    edges[0] = {{1, 1}};
    std::vector<std::int64_t> distance = gr::dijkstra(edges, 3);
    check_equal(distance[3], std::int64_t(0), "the chosen start is at zero");
    check_equal(distance[1], std::int64_t(2), "and distances follow from it");
    check_equal(distance[2], none, "the rest stays unreachable");
  }
}

void components_against_reachability() {
  testing::section("components against reachability");
  testing::random_source source(seed ^ 0x3333333333333333ULL);

  for (std::size_t size = 1; size <= 9; ++size) {
    for (std::size_t density : {std::size_t(1), std::size_t(3), std::size_t(8)}) {
      for (std::size_t attempt = 0; attempt < 6; ++attempt) {
        gr::graph edges = random_undirected(source, size, density);
        std::vector<std::int64_t> component = gr::get_components(edges);
        std::vector<std::vector<bool>> reaches = reachability(edges);

        check_equal(component.size(), size, "one index per vertex");

        for (std::size_t from = 0; from < size; ++from) {
          for (std::size_t to = 0; to < size; ++to) {
            check_equal(component[from] == component[to], reaches[from][to],
                        "same index exactly when connected, " + at(size, from, to));
          }
        }

        // Indices run from zero without gaps, and a component's index follows the order of its
        // smallest vertex.
        std::int64_t highest = -1;
        for (std::size_t vertex = 0; vertex < size; ++vertex) {
          check(component[vertex] >= 0, "every vertex has an index");
          check(component[vertex] <= highest + 1,
                "indices appear in order as vertices are scanned, n=" + std::to_string(size));
          highest = std::max(highest, component[vertex]);
        }
        check_equal(component[0], std::int64_t(0), "the first vertex is in component zero");
      }
    }
  }
}

void components_on_shapes() {
  testing::section("components on shapes");

  {
    gr::graph empty_graph;
    check(gr::get_components(empty_graph).empty(), "no vertices, no indices");
  }
  {
    gr::graph single(1);
    std::vector<std::int64_t> component = gr::get_components(single);
    check_equal(component.size(), std::size_t(1), "one vertex");
    check_equal(component[0], std::int64_t(0), "in component zero");
  }
  {
    gr::graph isolated(5);
    std::vector<std::int64_t> component = gr::get_components(isolated);
    for (std::size_t vertex = 0; vertex < 5; ++vertex) {
      check_equal(component[vertex], static_cast<std::int64_t>(vertex),
                  "isolated vertices are their own components, in order");
    }
  }
  {
    // Two pieces, with the larger one second, so numbering by order rather than by size shows.
    gr::graph split(5);
    split[0] = {1};
    split[1] = {0};
    split[2] = {3, 4};
    split[3] = {2, 4};
    split[4] = {2, 3};
    std::vector<std::int64_t> component = gr::get_components(split);
    check_equal(component[0], std::int64_t(0), "the piece holding vertex zero is numbered zero");
    check_equal(component[1], std::int64_t(0), "with its neighbour");
    check_equal(component[2], std::int64_t(1), "the next piece is numbered one");
    check_equal(component[4], std::int64_t(1), "throughout");
  }
  {
    // A long path: the iterative traversal must not care about depth.
    const std::size_t size = 100000;
    gr::graph path(size);
    for (std::size_t vertex = 0; vertex + 1 < size; ++vertex) {
      path[vertex].push_back(static_cast<std::int64_t>(vertex + 1));
      path[vertex + 1].push_back(static_cast<std::int64_t>(vertex));
    }
    std::vector<std::int64_t> component = gr::get_components(path);
    check_equal(component.front(), std::int64_t(0), "a long path is one component");
    check_equal(component.back(), std::int64_t(0), "from end to end");
  }
}

// A larger graph where the two algorithms must agree with each other, since Floyd-Warshall would
// be too slow to serve as the reference.
void the_two_algorithms_agree_at_scale() {
  testing::section("the two algorithms agree at scale");
  testing::random_source source(seed ^ 0x4444444444444444ULL);

  const std::size_t size = 300;
  gr::graph_weighted edges(size);
  for (std::size_t from = 0; from < size; ++from) {
    for (std::size_t attempt = 0; attempt < 4; ++attempt) {
      std::size_t to = source.below(size);
      if (to != from) {
        edges[from].emplace_back(static_cast<std::int64_t>(to),
                                 static_cast<std::int64_t>(source.below(1000)));
      }
    }
  }

  for (std::int64_t start : {std::int64_t(0), std::int64_t(7), std::int64_t(299)}) {
    std::vector<std::int64_t> fast = gr::dijkstra(edges, start);
    std::vector<std::int64_t> slow = gr::bellman_ford(edges, start);
    for (std::size_t finish = 0; finish < size; ++finish) {
      check_equal(fast[finish], slow[finish],
                  "dijkstra and bellman-ford agree, start=" + std::to_string(start) +
                      " finish=" + std::to_string(finish));
      check_equal(gr::dijkstra(edges, start, static_cast<std::int64_t>(finish)), fast[finish],
                  "and the early exit agrees with the full run");
    }
  }
}

}  // namespace

int main() {
  dijkstra_against_floyd_warshall();
  bellman_ford_against_floyd_warshall();
  negative_cycles_against_the_classification();
  negative_cycles_by_hand();
  unweighted_graphs_count_edges();
  degenerate_inputs();
  components_against_reachability();
  components_on_shapes();
  the_two_algorithms_agree_at_scale();

  return testing::summarize("shortest_paths") == 0 ? 0 : 1;
}
