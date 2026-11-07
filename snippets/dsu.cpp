// algo::dsu: disjoint set union, union by size + path compression, 0-indexed
namespace algo {

// Elements 0..n-1. Amortised almost O(1) per call. Arguments outside [0, n) are undefined behaviour.
class dsu {
 public:
  dsu() = default;

  // n singletons. O(n).
  explicit dsu(std::size_t n) : _parent(n), _size(n, 1), _components(n) {
    for (std::size_t v = 0; v < n; ++v) _parent[v] = v;
  }

  // Representative of v's set. Iterative, so no deep recursion.
  std::size_t find(std::size_t v) {
    std::size_t root = v;
    while (_parent[root] != root) root = _parent[root];
    while (_parent[v] != root) {
      std::size_t next = _parent[v];
      _parent[v] = root;
      v = next;
    }
    return root;
  }

  // Merges the sets of a and b. Returns false if they were already one set.
  bool unite(std::size_t a, std::size_t b) {
    a = find(a), b = find(b);
    if (a == b) return false;
    if (_size[a] < _size[b]) std::swap(a, b);
    _parent[b] = a;
    _size[a] += _size[b];
    --_components;
    return true;
  }

  bool same(std::size_t a, std::size_t b) { return find(a) == find(b); }

  // Size of v's set.
  std::size_t size(std::size_t v) { return _size[find(v)]; }

  // Number of sets.
  std::size_t components() const { return _components; }

 private:
  std::vector<std::size_t> _parent, _size;
  std::size_t _components = 0;
};

}  // namespace algo
