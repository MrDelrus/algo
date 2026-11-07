// segment tree node: sum monoid (edit value_type / identity / combine per problem)
struct monoid {
  using value_type = long long;
  static value_type identity() {
    return value_type(0);
  }
  static value_type combine(const value_type& left, const value_type& right) {
    return left + right;
  }
};
