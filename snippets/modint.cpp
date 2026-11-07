// algo::modint<M>: static modint, prime M, long long inside, invariant 0 <= v < M
#include <cassert>
namespace algo {

template <long long M>
struct modint {
  static_assert(1 <= M && M <= 3'037'000'499, "M * M must fit in long long");

  modint(long long x = 0) : v(x % M) {
    if (v < 0) v += M;
  }
  static modint raw(long long x) {
    assert(0 <= x && x < M);
    modint r;
    r.v = x;
    return r;
  }

  static constexpr long long mod() { return M; }
  long long val() const { return v; }

  modint& operator+=(const modint& o) {
    if ((v += o.v) >= M) v -= M;
    return *this;
  }
  modint& operator-=(const modint& o) {
    if ((v -= o.v) < 0) v += M;
    return *this;
  }
  modint& operator*=(const modint& o) {
    v = v * o.v % M;
    return *this;
  }
  modint& operator/=(const modint& o) { return *this *= o.inv(); }

  modint& operator++() { return *this += 1; }
  modint& operator--() { return *this -= 1; }
  modint operator++(int) {
    modint t = *this;
    ++*this;
    return t;
  }
  modint operator--(int) {
    modint t = *this;
    --*this;
    return t;
  }

  modint operator+() const { return *this; }
  modint operator-() const { return modint() - *this; }

  modint pow(long long n) const {
    assert(n >= 0);
    modint x = *this, r = 1;
    for (; n; n >>= 1, x *= x)
      if (n & 1) r *= x;
    return r;
  }
  modint inv() const {
    assert(v != 0);
    return pow(M - 2);
  }

  friend modint operator+(modint a, const modint& b) { return a += b; }
  friend modint operator-(modint a, const modint& b) { return a -= b; }
  friend modint operator*(modint a, const modint& b) { return a *= b; }
  friend modint operator/(modint a, const modint& b) { return a /= b; }
  friend bool operator==(const modint& a, const modint& b) { return a.v == b.v; }
  friend bool operator!=(const modint& a, const modint& b) { return a.v != b.v; }

  friend ostream& operator<<(ostream& os, const modint& a) { return os << a.v; }
  friend istream& operator>>(istream& is, modint& a) {
    long long x;
    is >> x;
    a = modint(x);
    return is;
  }

 private:
  long long v;
};

}  // namespace algo

using mint = algo::modint<1'000'000'007>;
