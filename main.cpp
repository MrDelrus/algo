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

// A monoid supplies an associative combine and a two-sided identity.
// Commutativity is not required: every structure that folds a range keeps operand order.

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

// Point assignment, range fold over an arbitrary monoid.
// Build O(n), get O(1), set O(log n), combine_at O(log n), query O(log n), query_all O(1).
// Memory 2 * ceil_pow2(n) values.
//
// The leaf count is rounded up to a power of two. Padding leaves hold identity(), which is why
// query(0, n) and query_all() agree.
//
// Ancestors are always recomputed from their two children, never patched in place. That is
// what lets combine_at work for operations with no inverse, min and max among them.
//
// Every entry point validates its arguments and throws std::out_of_range on misuse.
// A satisfied check is one predicted branch.
template <typename monoid>
class segment_tree {
 public:
  using value_type = typename monoid::value_type;

  segment_tree() = default;

  explicit segment_tree(std::size_t size)
      : _size(size), _leaves(leaf_count(size)), _tree(2 * _leaves, monoid::identity()) {}

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

  value_type get(std::size_t position) const {
    check_position(position, "segment_tree::get");
    return _tree[_leaves + position];
  }

  void set(std::size_t position, const value_type& value) {
    check_position(position, "segment_tree::set");
    std::size_t node = _leaves + position;
    _tree[node] = value;
    pull_up(node);
  }

  // a[position] = combine(a[position], value)
  void combine_at(std::size_t position, const value_type& value) {
    check_position(position, "segment_tree::combine_at");
    std::size_t node = _leaves + position;
    _tree[node] = monoid::combine(_tree[node], value);
    pull_up(node);
  }

  // Fold of [left, right). Returns identity() when left == right.
  value_type query(std::size_t left, std::size_t right) const {
    check_range(left, right, "segment_tree::query");
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

  value_type query_all() const {
    return _tree.empty() ? monoid::identity() : _tree[1];
  }

 private:
  static std::size_t leaf_count(std::size_t size) {
    return std::bit_ceil(size == 0 ? std::size_t(1) : size);
  }

  // Precondition checks. Building the message happens only on the failing path, so a
  // satisfied check costs one perfectly predicted branch.
  [[noreturn]] static void reject_position(const char* where, std::size_t position,
                                           std::size_t size) {
    throw std::out_of_range(std::string(where) + ": position " + std::to_string(position) +
                            " is out of range for a tree of size " + std::to_string(size));
  }

  [[noreturn]] static void reject_range(const char* where, std::size_t left, std::size_t right,
                                        std::size_t size) {
    throw std::out_of_range(std::string(where) + ": range [" + std::to_string(left) + ", " +
                            std::to_string(right) + ") is not inside [0, " + std::to_string(size) +
                            ")");
  }

  void check_position(std::size_t position, const char* where) const {
    if (position >= _size) {
      reject_position(where, position, _size);
    }
  }

  void check_range(std::size_t left, std::size_t right, const char* where) const {
    if (left > right || right > _size) {
      reject_range(where, left, right, _size);
    }
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

// Disjoint set union with union by size and full path compression.
// Construction O(n), every query and merge O(alpha(n)) amortised. Memory 2n values.
//
// Union by size rather than by rank: the bound is identical, both keep tree height O(log n)
// before compression, but a size is something problems ask for and a rank is not. After
// compression a rank stops being a real height anyway, while a size stays exact.
//
// get_ancestor rewrites the path it walks, yet it is const. Compression changes how the same
// partition is stored, never which partition it is, so it is invisible in the mathematical
// model the class presents. The storage is mutable for exactly that reason.
//
// Every entry point validates its arguments and throws std::out_of_range on misuse.
class disjoint_set_union {
 public:
  disjoint_set_union() = default;

  explicit disjoint_set_union(std::size_t size)
      : _parent(size), _component_size(size, 1), _component_count(size) {
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      _parent[vertex] = vertex;
    }
  }

  std::size_t get_ancestor(std::size_t vertex) const {
    check_vertex(vertex, "disjoint_set_union::get_ancestor");
    return find_root(vertex);
  }

  // Joins the two components. Returns false when the vertices already shared one, which is what
  // tells Kruskal that an edge closes a cycle.
  bool merge(std::size_t first, std::size_t second) {
    check_vertex(first, "disjoint_set_union::merge");
    check_vertex(second, "disjoint_set_union::merge");
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

  bool is_connected(std::size_t first, std::size_t second) const {
    check_vertex(first, "disjoint_set_union::is_connected");
    check_vertex(second, "disjoint_set_union::is_connected");
    return find_root(first) == find_root(second);
  }

  std::size_t get_component_size(std::size_t vertex) const {
    check_vertex(vertex, "disjoint_set_union::get_component_size");
    return _component_size[find_root(vertex)];
  }

  std::size_t get_component_count() const {
    return _component_count;
  }

 private:
  // Two passes: walk up to the root, then walk the same path again attaching every vertex
  // straight to it. No recursion — a path can be as long as the structure is wide, and a
  // Codeforces stack does not survive that.
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

  [[noreturn]] static void reject_vertex(const char* where, std::size_t vertex, std::size_t size) {
    throw std::out_of_range(std::string(where) + ": vertex " + std::to_string(vertex) +
                            " is out of range for a structure of size " + std::to_string(size));
  }

  void check_vertex(std::size_t vertex, const char* where) const {
    if (vertex >= _parent.size()) {
      reject_vertex(where, vertex, _parent.size());
    }
  }

  mutable std::vector<std::size_t> _parent;
  std::vector<std::size_t> _component_size;
  std::size_t _component_count = 0;
};

}  // namespace disjoint_set_unions

// Every structure the library offers, gathered in one place. Reach past an alias only for a
// monoid that has no preset: ds::segment_trees::segment_tree<my_monoid>.

using sum_segment_tree = segment_trees::segment_tree<sum_monoid<std::int64_t>>;
using min_segment_tree = segment_trees::segment_tree<min_monoid<std::int64_t>>;
using max_segment_tree = segment_trees::segment_tree<max_monoid<std::int64_t>>;
using gcd_segment_tree = segment_trees::segment_tree<gcd_monoid<std::int64_t>>;

using disjoint_set_union = disjoint_set_unions::disjoint_set_union;

}  // namespace data_structures

}  // namespace algo

namespace ds = algo::data_structures;

void solve() {



}

int main() {
  fast;

  int tt;
  cin >> tt;
  while (tt--) {
    solve();
  }

  return 0;
}
