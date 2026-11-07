// algo::segment_tree<monoid>: point set, range fold (C++20)
namespace algo {

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

  // Number of elements. O(1).
  std::size_t size() const {
    return _size;
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

}  // namespace algo
