// algo::sparse_table<T, op>: static range query for an idempotent op (min, max, gcd, and, or), O(1) query (C++20)
namespace algo {

// Built once, never changed. op must be associative and idempotent: op(x, x) == x.
// Positions 0-indexed, ranges half-open, query needs left < right.
// Example: algo::sparse_table<int, [](int a, int b) { return std::min(a, b); }> st(a);
template <typename T, auto op>
class sparse_table {
 public:
  sparse_table() = default;

  // O(n log n) time and memory.
  explicit sparse_table(const std::vector<T>& values) : _size(values.size()) {
    std::size_t levels = std::bit_width(std::max<std::size_t>(_size, 1));
    _table.assign(levels, values);
    for (std::size_t k = 1; k < levels; ++k) {
      std::size_t half = std::size_t(1) << (k - 1);
      _table[k].resize(_size - (std::size_t(1) << k) + 1);
      for (std::size_t i = 0; i < _table[k].size(); ++i) {
        _table[k][i] = op(_table[k - 1][i], _table[k - 1][i + half]);
      }
    }
  }

  std::size_t size() const { return _size; }

  // op over [left, right), left < right. O(1).
  T query(std::size_t left, std::size_t right) const {
    std::size_t k = std::bit_width(right - left) - 1;
    return op(_table[k][left], _table[k][right - (std::size_t(1) << k)]);
  }

 private:
  std::size_t _size = 0;
  std::vector<std::vector<T>> _table;
};

}  // namespace algo
