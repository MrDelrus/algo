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

namespace cartesian_trees {

// A cartesian tree, also called a treap: a binary search tree on keys and a heap on priorities
// drawn at random when a key is inserted. The randomness is what keeps it balanced — no
// rotations, no colour rules, and an expected depth of O(log n) whatever order the keys arrive
// in. Every operation is O(log n) expected; none is O(log n) guaranteed.
//
// The interface is a set's — insert, erase, contains. What a set cannot do, and the reason this
// exists, is split and merge: cutting the tree at a key and joining two trees, each in O(log n)
// rather than by rebuilding.
//
// **Not a faster std::set.** Measured on a mix of insertions, lookups and erasures, std::set
// wins: a red-black tree guarantees its depth where a treap only expects it, and libstdc++ has
// spent decades on node allocation. Reach for this when a problem needs split or merge, and for
// std::set when it does not.
//
// Only operator< is required of a key, exactly as std::set requires. Equality is expressed as
// neither ordering holding.
//
// Nodes are held by std::unique_ptr, so a tree owns its nodes, copies deeply and moves cheaply.
// A flat arena with indices measured about 1.3x faster, and was rejected: it would have forced
// a shared static pool, and with it a type that cannot be copied and whose lifetime is tangled
// with every other tree of the same key. That price buys less than std::set gives away for
// free, so it is not worth paying.
template <typename key_type>
class cartesian_tree {
 public:
  cartesian_tree() : _priority_state(seed_from_clock()) {}

  // Fixing the seed makes a run reproducible, which is what tests need. Priorities are only
  // ever compared inside one tree, so trees sharing a seed cost nothing.
  explicit cartesian_tree(std::uint64_t seed) : _priority_state(seed) {}

  cartesian_tree(const cartesian_tree& other)
      : _root(clone(other._root.get())), _priority_state(other._priority_state) {}

  cartesian_tree& operator=(const cartesian_tree& other) {
    if (this != &other) {
      _root = clone(other._root.get());
      _priority_state = other._priority_state;
    }
    return *this;
  }

  // A moved-from tree is left empty rather than merely unspecified. split and merge hand their
  // nodes to the result, and the source has to stop claiming them.
  cartesian_tree(cartesian_tree&& other) noexcept
      : _root(std::move(other._root)), _priority_state(other._priority_state) {}

  cartesian_tree& operator=(cartesian_tree&& other) noexcept {
    if (this != &other) {
      _root = std::move(other._root);
      _priority_state = other._priority_state;
    }
    return *this;
  }

  ~cartesian_tree() = default;

  std::size_t size() const {
    return subtree_size(_root.get());
  }

  bool empty() const {
    return _root == nullptr;
  }

  bool contains(const key_type& key) const {
    const node* current = _root.get();
    while (current != nullptr) {
      if (key < current->key) {
        current = current->left.get();
      } else if (current->key < key) {
        current = current->right.get();
      } else {
        return true;
      }
    }
    return false;
  }

  // Returns false when the key was already present, like std::set::insert.
  bool insert(const key_type& key) {
    if (contains(key)) {
      return false;
    }
    std::unique_ptr<node> less;
    std::unique_ptr<node> not_less;
    split_nodes(std::move(_root), key, less, not_less);
    std::unique_ptr<node> fresh(new node{key, next_priority(), 1, nullptr, nullptr});
    _root = merge_nodes(merge_nodes(std::move(less), std::move(fresh)), std::move(not_less));
    return true;
  }

  // Returns false when the key was not there.
  bool erase(const key_type& key) {
    return erase_node(_root, key);
  }

 private:
  struct node {
    key_type key;
    std::uint64_t priority;
    std::size_t subtree_size;
    std::unique_ptr<node> left;
    std::unique_ptr<node> right;
  };

  static std::size_t subtree_size(const node* tree) {
    return tree == nullptr ? 0 : tree->subtree_size;
  }

  // Recomputes what a node knows about its subtree from its two children. Everything derived
  // lives here: the size today, and a monoid aggregate on the day one is added.
  static void pull(node* tree) {
    tree->subtree_size = 1 + subtree_size(tree->left.get()) + subtree_size(tree->right.get());
  }

  // Splits into keys < key and keys >= key. Both halves may be empty.
  static void split_nodes(std::unique_ptr<node> tree, const key_type& key,
                          std::unique_ptr<node>& less, std::unique_ptr<node>& not_less) {
    if (tree == nullptr) {
      less.reset();
      not_less.reset();
      return;
    }
    if (tree->key < key) {
      std::unique_ptr<node> detached = std::move(tree->right);
      split_nodes(std::move(detached), key, tree->right, not_less);
      pull(tree.get());
      less = std::move(tree);
    } else {
      std::unique_ptr<node> detached = std::move(tree->left);
      split_nodes(std::move(detached), key, less, tree->left);
      pull(tree.get());
      not_less = std::move(tree);
    }
  }

  // Assumes every key on the left is smaller than every key on the right. The public merge
  // checks that; this does not, because split has just guaranteed it.
  static std::unique_ptr<node> merge_nodes(std::unique_ptr<node> left,
                                           std::unique_ptr<node> right) {
    if (left == nullptr) {
      return right;
    }
    if (right == nullptr) {
      return left;
    }
    if (left->priority > right->priority) {
      std::unique_ptr<node> merged = merge_nodes(std::move(left->right), std::move(right));
      left->right = std::move(merged);
      pull(left.get());
      return left;
    }
    std::unique_ptr<node> merged = merge_nodes(std::move(left), std::move(right->left));
    right->left = std::move(merged);
    pull(right.get());
    return right;
  }

  static bool erase_node(std::unique_ptr<node>& tree, const key_type& key) {
    if (tree == nullptr) {
      return false;
    }
    if (!(key < tree->key) && !(tree->key < key)) {
      std::unique_ptr<node> joined = merge_nodes(std::move(tree->left), std::move(tree->right));
      tree = std::move(joined);
      return true;
    }
    bool erased = erase_node(key < tree->key ? tree->left : tree->right, key);
    if (erased) {
      pull(tree.get());
    }
    return erased;
  }

  static std::unique_ptr<node> clone(const node* tree) {
    if (tree == nullptr) {
      return nullptr;
    }
    std::unique_ptr<node> copy(new node{tree->key, tree->priority, tree->subtree_size,
                                        clone(tree->left.get()), clone(tree->right.get())});
    return copy;
  }

  static const key_type& smallest_key(const node* tree) {
    while (tree->left != nullptr) {
      tree = tree->left.get();
    }
    return tree->key;
  }

  static const key_type& largest_key(const node* tree) {
    while (tree->right != nullptr) {
      tree = tree->right.get();
    }
    return tree->key;
  }

  static std::uint64_t seed_from_clock() {
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  std::uint64_t next_priority() {
    _priority_state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t mixed = _priority_state;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebULL;
    return mixed ^ (mixed >> 31);
  }

  std::unique_ptr<node> _root;
  std::uint64_t _priority_state = 0;

  template <typename other_key>
  friend std::pair<cartesian_tree<other_key>, cartesian_tree<other_key>> split(
      cartesian_tree<other_key>&& tree, const std::type_identity_t<other_key>& key);

  template <typename other_key>
  friend cartesian_tree<other_key> merge(cartesian_tree<other_key>&& left,
                                         cartesian_tree<other_key>&& right);
};

// The key is deliberately not deduced — std::type_identity_t makes the tree alone decide the
// type, so split(std::move(tree), 6) works on a tree of std::int64_t instead of failing because
// a bare literal has a narrower type of its own.
//
// Cuts the tree in two: keys < key on the left, keys >= key on the right. Either half may come
// back empty. The source is consumed — its nodes end up in the results — so it is left empty,
// and the std::move at the call site is the only thing in the code that says so.
template <typename key_type>
std::pair<cartesian_tree<key_type>, cartesian_tree<key_type>> split(
    cartesian_tree<key_type>&& tree, const std::type_identity_t<key_type>& key) {
  cartesian_tree<key_type> less(tree._priority_state);
  cartesian_tree<key_type> not_less(tree._priority_state);
  cartesian_tree<key_type>::split_nodes(std::move(tree._root), key, less._root, not_less._root);
  return std::pair<cartesian_tree<key_type>, cartesian_tree<key_type>>(std::move(less),
                                                                       std::move(not_less));
}

// Joins two trees, and requires every key on the left to be smaller than every key on the right;
// throws std::invalid_argument otherwise. The condition is strict rather than <=, because equal
// keys at the seam would put the same key in the tree twice and quietly break the set invariant
// that insert and erase rely on. A merge of two halves that came from split always satisfies it.
template <typename key_type>
cartesian_tree<key_type> merge(cartesian_tree<key_type>&& left, cartesian_tree<key_type>&& right) {
  using tree_type = cartesian_tree<key_type>;
  if (left._root != nullptr && right._root != nullptr) {
    const key_type& boundary_left = tree_type::largest_key(left._root.get());
    const key_type& boundary_right = tree_type::smallest_key(right._root.get());
    if (!(boundary_left < boundary_right)) {
      throw std::invalid_argument(
          "cartesian_trees::merge: every key on the left must be smaller than every key on the "
          "right");
    }
  }
  tree_type result(left._priority_state);
  result._root = tree_type::merge_nodes(std::move(left._root), std::move(right._root));
  return result;
}

}  // namespace cartesian_trees

// Every structure the library offers, gathered in one place. Reach past an alias only for a
// monoid that has no preset: ds::segment_trees::segment_tree<my_monoid>.

using sum_segment_tree = segment_trees::segment_tree<sum_monoid<std::int64_t>>;
using min_segment_tree = segment_trees::segment_tree<min_monoid<std::int64_t>>;
using max_segment_tree = segment_trees::segment_tree<max_monoid<std::int64_t>>;
using gcd_segment_tree = segment_trees::segment_tree<gcd_monoid<std::int64_t>>;

using disjoint_set_union = disjoint_set_unions::disjoint_set_union;

template <typename key_type>
using cartesian_tree = cartesian_trees::cartesian_tree<key_type>;

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
