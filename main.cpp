#include <bits/stdc++.h>
using namespace std;

#define all(a) a.begin(), a.end()
#define rall(a) a.rbegin(), a.rend()
#define max_v(v) *max_element(all(v))
#define min_v(v) *min_element(all(v))
#define sz(v) static_cast<ll>((v).size())
#define pb push_back
#define eb emplace_back

using ll = long long;
using ull = unsigned long long;
using ld = long double;
using pll = pair<ll, ll>;
using vll = vector<ll>;
using vvll = vector<vector<ll>>;
template <typename T>
using min_heap = priority_queue<T, vector<T>, greater<T>>;

constexpr ll INF = numeric_limits<ll>::max() / 4;

struct custom_hash {
  static uint64_t splitmix64(uint64_t x) {
    // http://xorshift.di.unimi.it/splitmix64.c
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
  }

  size_t operator()(uint64_t x) const {
    static const uint64_t FIXED_RANDOM = chrono::steady_clock::now().time_since_epoch().count();
    return splitmix64(x + FIXED_RANDOM);
  }
};

template <typename K, typename V>
using hash_map = unordered_map<K, V, custom_hash>;
template <typename K>
using hash_set = unordered_set<K, custom_hash>;

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

#ifdef LOCAL
#define dbg(...) (cerr << "[" << #__VA_ARGS__ << "] = ", debug_out(__VA_ARGS__))
inline void debug_out() {
  cerr << '\n';
}
template <typename T, typename... Rest>
void debug_out(const T& x, const Rest&... rest) {
  cerr << x;
  if (sizeof...(rest))
    cerr << ", ";
  debug_out(rest...);
}
template <typename T>
ostream& operator<<(ostream& out, const vector<T>& v) {
  out << '{';
  for (size_t i = 0; i < v.size(); ++i)
    out << (i ? ", " : "") << v[i];
  return out << '}';
}
template <typename A, typename B>
ostream& operator<<(ostream& out, const pair<A, B>& p) {
  return out << '(' << p.first << ", " << p.second << ')';
}
#else
#define dbg(...) ((void)0)
#endif

// STANDARD ALGORITHMS

// CODE HERE

int main() {
  ios_base::sync_with_stdio(false);
  cin.tie(nullptr);
  cout.tie(nullptr);

  // ifstream cin("input.txt"); ofstream cout("output.txt");

  int tt;
  cin >> tt;  // DELETE THIS LINE IF THE PROBLEM HAS A SINGLE TEST

  while (tt--) {
  }

  return 0;
}
