# Shortest paths and components

Dijkstra, Bellman-Ford and connected components over adjacency lists.

## Graph types

```cpp
gr::graph edges(n);                       // std::vector<std::vector<std::int64_t>>
edges[from].push_back(to);

gr::graph_weighted edges(n);              // ... of std::pair<destination, weight>
edges[from].emplace_back(to, weight);
```

Vertices are `0 .. n - 1`, and `n` is `edges.size()`. Both types are directed: an undirected edge is two entries. Weights and distances are `std::int64_t`.

`gr::unreachable` is the distance reported where no path exists. It is `std::numeric_limits<std::int64_t>::max()`.

## Distances

Every function takes either graph type. An edge of an unweighted `graph` weighs one.

| | |
| --- | --- |
| `dijkstra(edges, start)` | Distances from `start`. **Weights must be non-negative.** O((n + m) log n). |
| `dijkstra(edges, start, finish)` | The one distance, returning as soon as `finish` is settled. O((n + m) log n). |
| `bellman_ford(edges, start)` | Distances from `start`, negative weights allowed. **Returns an empty vector when a negative cycle is reachable from `start`.** O(n · m). |
| `bellman_ford(edges, start, finish)` | The one distance; `unreachable` when a negative cycle is reachable. O(n · m). |

```cpp
std::vector<std::int64_t> distance = gr::dijkstra(edges, 0);
if (distance[target] == gr::unreachable) { /* no path */ }

std::int64_t answer = gr::dijkstra(edges, source, target);

std::vector<std::int64_t> reached = gr::bellman_ford(edges, 0);
if (reached.empty()) { /* negative cycle */ }
```

## Components

```cpp
std::vector<std::int64_t> component = gr::get_components(edges);
```

One index per vertex, from `0` to the number of components minus one. Two vertices carry the same index exactly when a path joins them, so edges are read as **undirected** — put both directions in the lists.

Components are numbered by the order of their smallest vertex: vertex 0 is always in component 0, and scanning vertices upwards meets component indices in increasing order. O(n + m), iterative, so depth does not matter.

## Traps

- **Dijkstra with a negative weight gives a wrong answer silently.** Nothing checks. Use `bellman_ford`.
- **A negative cycle that `start` cannot reach is not reported**, and the distances returned are correct. Only a reachable one produces the empty vector.
- **`get_components` is not strongly connected components.** On a directed graph it answers a question about neither direction.
- **Distances are sums of `std::int64_t`.** A path whose total exceeds the range wraps; nothing checks.
- On an unweighted graph `dijkstra` costs a logarithmic factor that a breadth-first search would not.

## Related

- [documentation index](../README.md)
- [tests](../../tests/graphs/shortest_paths.cpp)
