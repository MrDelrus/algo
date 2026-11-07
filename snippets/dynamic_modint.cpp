// algo::dynamic_modint: modulus set at runtime (set_mod), long long inside, invariant 0 <= v < m
#include <cassert>
namespace algo {

struct dynamic_modint {
  // Call before creating any value. Forgetting it divides by zero -> runtime error.
  static void set_mod(long long m) {
    assert(1 <= m && m <= 3'037'000'499);  // m * m must fit in long long
    _mod = m;
  }
  static long long mod() { return _mod; }

  dynamic_modint(long long x = 0) : v(x % _mod) {
    if (v < 0) v += _mod;
  }
  static dynamic_modint raw(long long x) {
    assert(0 <= x && x < _mod);
    dynamic_modint r;
    r.v = x;
    return r;
  }

  long long val() const { return v; }

  dynamic_modint& operator+=(const dynamic_modint& o) {
    if ((v += o.v) >= _mod) v -= _mod;
    return *this;
  }
  dynamic_modint& operator-=(const dynamic_modint& o) {
    if ((v -= o.v) < 0) v += _mod;
    return *this;
  }
  dynamic_modint& operator*=(const dynamic_modint& o) {
    v = v * o.v % _mod;
    return *this;
  }
  dynamic_modint& operator/=(const dynamic_modint& o) { return *this *= o.inv(); }

  dynamic_modint& operator++() { return *this += 1; }
  dynamic_modint& operator--() { return *this -= 1; }
  dynamic_modint operator++(int) {
    dynamic_modint t = *this;
    ++*this;
    return t;
  }
  dynamic_modint operator--(int) {
    dynamic_modint t = *this;
    --*this;
    return t;
  }

  dynamic_modint operator+() const { return *this; }
  dynamic_modint operator-() const { return dynamic_modint() - *this; }

  dynamic_modint pow(long long n) const {
    assert(n >= 0);
    dynamic_modint x = *this, r = 1;
    for (; n; n >>= 1, x *= x)
      if (n & 1) r *= x;
    return r;
  }
  // Extended Euclid: works for any modulus, needs gcd(v, m) == 1.
  dynamic_modint inv() const {
    long long a = v, b = _mod, x = 1, y = 0;  // invariant: a = x * v, b = y * v (mod m)
    while (b) {
      long long q = a / b;
      a -= q * b, x -= q * y;
      std::swap(a, b), std::swap(x, y);
    }
    assert(a == 1);
    return dynamic_modint(x);
  }

  friend dynamic_modint operator+(dynamic_modint a, const dynamic_modint& b) { return a += b; }
  friend dynamic_modint operator-(dynamic_modint a, const dynamic_modint& b) { return a -= b; }
  friend dynamic_modint operator*(dynamic_modint a, const dynamic_modint& b) { return a *= b; }
  friend dynamic_modint operator/(dynamic_modint a, const dynamic_modint& b) { return a /= b; }
  friend bool operator==(const dynamic_modint& a, const dynamic_modint& b) { return a.v == b.v; }
  friend bool operator!=(const dynamic_modint& a, const dynamic_modint& b) { return a.v != b.v; }

  friend std::ostream& operator<<(std::ostream& os, const dynamic_modint& a) { return os << a.v; }
  friend std::istream& operator>>(std::istream& is, dynamic_modint& a) {
    long long x;
    is >> x;
    a = dynamic_modint(x);
    return is;
  }

 private:
  long long v;
  inline static long long _mod = 0;
};

}  // namespace algo
