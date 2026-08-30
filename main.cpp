// https://github.com/MrDelrus/algo
#include <bits/stdc++.h>
using namespace std;

#define all(a) a.begin(), a.end()
#define min_v(v) *min_element(all(v))
#define max_v(v) *max_element(all(v))
#define pb push_back
#define eb emplace_back

#define fast ios_base::sync_with_stdio(0), cin.tie(0), cout.tie(0)

using ll = int64_t;
using ld = long double;
using pll = pair<ll, ll>;

namespace algo {

namespace data_structures {

// A monoid: associative combine, two-sided identity. Folds keep operand order, so combine need
// not be commutative.

template <typename value>
struct sum_monoid {
  using value_type = value;
  static constexpr value_type identity() {
    return value_type(0);
  }
  static constexpr value_type combine(const value_type& left, const value_type& right) {
    return left + right;
  }
};

template <typename value>
struct min_monoid {
  using value_type = value;
  static constexpr value_type identity() {
    return std::numeric_limits<value_type>::max();
  }
  static constexpr value_type combine(const value_type& left, const value_type& right) {
    return right < left ? right : left;
  }
};

template <typename value>
struct max_monoid {
  using value_type = value;
  static constexpr value_type identity() {
    return std::numeric_limits<value_type>::lowest();
  }
  static constexpr value_type combine(const value_type& left, const value_type& right) {
    return left < right ? right : left;
  }
};

template <typename value>
struct gcd_monoid {
  using value_type = value;
  static constexpr value_type identity() {
    return value_type(0);
  }
  static value_type combine(const value_type& left, const value_type& right) {
    return std::gcd(left, right);
  }
};

namespace segment_trees {

// Point assignment, range fold over a monoid.
// Positions 0-indexed, ranges half-open. Arguments outside the tree are undefined behaviour.
template <typename monoid>
class segment_tree {
 public:
  using value_type = typename monoid::value_type;

  segment_tree() = default;

  // size elements, each identity(). O(n).
  explicit segment_tree(std::size_t size)
      : _size(size), _leaves(leaf_count(size)), _tree(2 * _leaves, monoid::identity()) {}

  // O(n).
  explicit segment_tree(const std::vector<value_type>& values)
      : _size(values.size()),
        _leaves(leaf_count(values.size())),
        _tree(2 * _leaves, monoid::identity()) {
    for (std::size_t position = 0; position < _size; ++position) {
      _tree[_leaves + position] = values[position];
    }
    for (std::size_t node = _leaves; node-- > 1;) {
      _tree[node] = monoid::combine(_tree[2 * node], _tree[2 * node + 1]);
    }
  }

  // The element at position. O(1).
  value_type get(std::size_t position) const {
    return _tree[_leaves + position];
  }

  // Assigns value, ignoring what was there. O(log n).
  void set(std::size_t position, const value_type& value) {
    std::size_t node = _leaves + position;
    _tree[node] = value;
    pull_up(node);
  }

  // a[position] = combine(a[position], value). O(log n).
  void combine_at(std::size_t position, const value_type& value) {
    std::size_t node = _leaves + position;
    _tree[node] = monoid::combine(_tree[node], value);
    pull_up(node);
  }

  // Fold of [left, right); identity() when left == right. O(log n).
  value_type query(std::size_t left, std::size_t right) const {
    value_type from_left = monoid::identity();
    value_type from_right = monoid::identity();
    for (std::size_t low = left + _leaves, high = right + _leaves; low < high;
         low /= 2, high /= 2) {
      if (low % 2 == 1) {
        from_left = monoid::combine(from_left, _tree[low]);
        ++low;
      }
      if (high % 2 == 1) {
        --high;
        from_right = monoid::combine(_tree[high], from_right);
      }
    }
    return monoid::combine(from_left, from_right);
  }

  // Fold of everything, equal to query(0, n). O(1).
  value_type query_all() const {
    return _tree.empty() ? monoid::identity() : _tree[1];
  }

 private:
  static std::size_t leaf_count(std::size_t size) {
    return std::bit_ceil(size == 0 ? std::size_t(1) : size);
  }

  void pull_up(std::size_t node) {
    while (node > 1) {
      node /= 2;
      _tree[node] = monoid::combine(_tree[2 * node], _tree[2 * node + 1]);
    }
  }

  std::size_t _size = 0;
  std::size_t _leaves = 0;
  std::vector<value_type> _tree;
};

}  // namespace segment_trees

namespace disjoint_set_unions {

// A partition of 0 .. n - 1 under merging. Vertices 0-indexed.
// Vertices outside the structure are undefined behaviour.
class disjoint_set_union {
 public:
  disjoint_set_union() = default;

  // size vertices, each alone. O(n).
  explicit disjoint_set_union(std::size_t size)
      : _parent(size), _component_size(size, 1), _component_count(size) {
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      _parent[vertex] = vertex;
    }
  }

  // The component's representative. Which vertex is unspecified and changes on merges.
  // O(alpha(n)) amortised.
  std::size_t get_ancestor(std::size_t vertex) const {
    return find_root(vertex);
  }

  // Joins two components; false when they already shared one. O(alpha(n)) amortised.
  bool merge(std::size_t first, std::size_t second) {
    std::size_t first_root = find_root(first);
    std::size_t second_root = find_root(second);
    if (first_root == second_root) {
      return false;
    }
    if (_component_size[first_root] < _component_size[second_root]) {
      std::size_t swapped = first_root;
      first_root = second_root;
      second_root = swapped;
    }
    _parent[second_root] = first_root;
    _component_size[first_root] += _component_size[second_root];
    --_component_count;
    return true;
  }

  // Whether the two vertices share a component. O(alpha(n)) amortised.
  bool is_connected(std::size_t first, std::size_t second) const {
    return find_root(first) == find_root(second);
  }

  // Size of this vertex's component. O(alpha(n)) amortised.
  std::size_t get_component_size(std::size_t vertex) const {
    return _component_size[find_root(vertex)];
  }

  // Components remaining. O(1).
  std::size_t get_component_count() const {
    return _component_count;
  }

 private:
  std::size_t find_root(std::size_t vertex) const {
    std::size_t root = vertex;
    while (_parent[root] != root) {
      root = _parent[root];
    }
    while (_parent[vertex] != root) {
      std::size_t next = _parent[vertex];
      _parent[vertex] = root;
      vertex = next;
    }
    return root;
  }

  mutable std::vector<std::size_t> _parent;
  std::vector<std::size_t> _component_size;
  std::size_t _component_count = 0;
};

}  // namespace disjoint_set_unions

// Priority queues: heap_min keeps the smallest element on top, heap_max the largest.
template <typename value_type>
using heap_min = std::priority_queue<value_type, std::vector<value_type>, std::greater<value_type>>;
template <typename value_type>
using heap_max = std::priority_queue<value_type>;

// A heap holding values, built in O(n). heapify(values) is a heap_min,
// heapify<heap_max>(values) a heap_max.
template <template <typename> class heap_kind = heap_min, typename value_type>
heap_kind<value_type> heapify(const std::vector<value_type>& values) {
  return heap_kind<value_type>(values.begin(), values.end());
}

// Takes ownership of values. O(n).
template <template <typename> class heap_kind = heap_min, typename value_type>
heap_kind<value_type> heapify(std::vector<value_type>&& values) {
  return heap_kind<value_type>(typename heap_kind<value_type>::value_compare(), std::move(values));
}

// Everything the library offers. Name the core only for a monoid with no preset here:
// ds::segment_trees::segment_tree<my_monoid>.

using segment_tree_sum = segment_trees::segment_tree<sum_monoid<std::int64_t>>;
using segment_tree_min = segment_trees::segment_tree<min_monoid<std::int64_t>>;
using segment_tree_max = segment_trees::segment_tree<max_monoid<std::int64_t>>;
using segment_tree_gcd = segment_trees::segment_tree<gcd_monoid<std::int64_t>>;

using disjoint_set_union = disjoint_set_unions::disjoint_set_union;

}  // namespace data_structures

namespace graphs {

// Adjacency lists over vertices 0 .. n - 1.
using graph = std::vector<std::vector<std::int64_t>>;

// Adjacency lists carrying weights: each pair is (destination, weight).
using graph_weighted = std::vector<std::vector<std::pair<std::int64_t, std::int64_t>>>;

// The distance reported for a vertex no path reaches.
inline constexpr std::int64_t unreachable = std::numeric_limits<std::int64_t>::max();

// The distance reported where paths exist but no shortest one does, because a negative cycle
// lies on the way. Only bellman_ford reports it.
inline constexpr std::int64_t unbounded_negative = std::numeric_limits<std::int64_t>::min();

namespace detail {

inline std::pair<std::int64_t, std::int64_t> as_edge(std::int64_t destination) {
  return {destination, 1};
}

inline std::pair<std::int64_t, std::int64_t> as_edge(
    const std::pair<std::int64_t, std::int64_t>& edge) {
  return edge;
}

}  // namespace detail

// Distances from start; unreachable where no path exists. Edges of a graph weigh one.
// Weights must be non-negative. O((n + m) log n).
template <typename graph_type>
std::vector<std::int64_t> dijkstra(const graph_type& edges, std::int64_t start) {
  std::vector<std::int64_t> distance(edges.size(), unreachable);
  distance[static_cast<std::size_t>(start)] = 0;
  data_structures::heap_min<std::pair<std::int64_t, std::int64_t>> queue;
  queue.emplace(0, start);

  while (!queue.empty()) {
    auto [reached, vertex] = queue.top();
    queue.pop();
    if (reached != distance[static_cast<std::size_t>(vertex)]) {
      continue;
    }
    for (const auto& neighbour : edges[static_cast<std::size_t>(vertex)]) {
      auto [destination, weight] = detail::as_edge(neighbour);
      std::int64_t candidate = reached + weight;
      if (candidate < distance[static_cast<std::size_t>(destination)]) {
        distance[static_cast<std::size_t>(destination)] = candidate;
        queue.emplace(candidate, destination);
      }
    }
  }
  return distance;
}

// The distance from start to finish alone, stopping as soon as it is settled; unreachable when
// no path exists. Weights must be non-negative. O((n + m) log n).
template <typename graph_type>
std::int64_t dijkstra(const graph_type& edges, std::int64_t start, std::int64_t finish) {
  std::vector<std::int64_t> distance(edges.size(), unreachable);
  distance[static_cast<std::size_t>(start)] = 0;
  data_structures::heap_min<std::pair<std::int64_t, std::int64_t>> queue;
  queue.emplace(0, start);

  while (!queue.empty()) {
    auto [reached, vertex] = queue.top();
    queue.pop();
    if (vertex == finish) {
      return reached;
    }
    if (reached != distance[static_cast<std::size_t>(vertex)]) {
      continue;
    }
    for (const auto& neighbour : edges[static_cast<std::size_t>(vertex)]) {
      auto [destination, weight] = detail::as_edge(neighbour);
      std::int64_t candidate = reached + weight;
      if (candidate < distance[static_cast<std::size_t>(destination)]) {
        distance[static_cast<std::size_t>(destination)] = candidate;
        queue.emplace(candidate, destination);
      }
    }
  }
  return unreachable;
}

// Distances from start, admitting negative weights. A vertex is unreachable where no path
// exists, and unbounded_negative where one exists but no shortest one does, a negative cycle lying
// on the way. Edges of a graph weigh one. O(n * m).
template <typename graph_type>
std::vector<std::int64_t> bellman_ford(const graph_type& edges, std::int64_t start) {
  std::vector<std::int64_t> distance(edges.size(), unreachable);
  if (edges.empty()) {
    return distance;
  }
  distance[static_cast<std::size_t>(start)] = 0;

  for (std::size_t pass = 0; pass + 1 < edges.size(); ++pass) {
    bool relaxed = false;
    for (std::size_t vertex = 0; vertex < edges.size(); ++vertex) {
      if (distance[vertex] == unreachable) {
        continue;
      }
      for (const auto& neighbour : edges[vertex]) {
        auto [destination, weight] = detail::as_edge(neighbour);
        std::int64_t candidate = distance[vertex] + weight;
        if (candidate < distance[static_cast<std::size_t>(destination)]) {
          distance[static_cast<std::size_t>(destination)] = candidate;
          relaxed = true;
        }
      }
    }
    if (!relaxed) {
      return distance;
    }
  }

  // Whatever still relaxes after n - 1 passes sits on a negative cycle, and so does everything
  // the cycle can reach.
  std::vector<std::int64_t> pending;
  for (std::size_t vertex = 0; vertex < edges.size(); ++vertex) {
    if (distance[vertex] == unreachable) {
      continue;
    }
    for (const auto& neighbour : edges[vertex]) {
      auto [destination, weight] = detail::as_edge(neighbour);
      if (distance[vertex] + weight < distance[static_cast<std::size_t>(destination)]) {
        pending.push_back(destination);
      }
    }
  }
  for (std::int64_t vertex : pending) {
    distance[static_cast<std::size_t>(vertex)] = unbounded_negative;
  }
  while (!pending.empty()) {
    std::int64_t vertex = pending.back();
    pending.pop_back();
    for (const auto& neighbour : edges[static_cast<std::size_t>(vertex)]) {
      auto [destination, weight] = detail::as_edge(neighbour);
      static_cast<void>(weight);
      if (distance[static_cast<std::size_t>(destination)] != unbounded_negative) {
        distance[static_cast<std::size_t>(destination)] = unbounded_negative;
        pending.push_back(destination);
      }
    }
  }
  return distance;
}

// The distance from start to finish alone: unreachable, unbounded_negative, or the length.
// O(n * m).
template <typename graph_type>
std::int64_t bellman_ford(const graph_type& edges, std::int64_t start, std::int64_t finish) {
  return bellman_ford(edges, start)[static_cast<std::size_t>(finish)];
}

// A component index per vertex, from 0 to the number of components minus one. Two vertices
// share an index exactly when a path joins them, so edges are read as undirected. Components
// are numbered by the order of their smallest vertex. O(n + m).
inline std::vector<std::int64_t> get_components(const graph& edges) {
  std::vector<std::int64_t> component(edges.size(), -1);
  std::vector<std::int64_t> pending;
  std::int64_t next = 0;

  for (std::size_t root = 0; root < edges.size(); ++root) {
    if (component[root] != -1) {
      continue;
    }
    component[root] = next;
    pending.push_back(static_cast<std::int64_t>(root));
    while (!pending.empty()) {
      std::int64_t vertex = pending.back();
      pending.pop_back();
      for (std::int64_t destination : edges[static_cast<std::size_t>(vertex)]) {
        if (component[static_cast<std::size_t>(destination)] == -1) {
          component[static_cast<std::size_t>(destination)] = next;
          pending.push_back(destination);
        }
      }
    }
    ++next;
  }
  return component;
}

}  // namespace graphs

}  // namespace algo

namespace ds = algo::data_structures;
namespace gr = algo::graphs;

void solve() {



}

int main() {
  fast;

  int tt = 1;
  cin >> tt;
  while (tt--) {
    solve();
  }

  return 0;
}
