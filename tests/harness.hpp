// Minimal assertion harness shared by every test file.
//
// Deliberately tiny: no framework, no dependencies, nothing to install. A test binary reports
// each failure with the section it happened in and a description precise enough to locate the
// bug without a debugger, then exits non-zero if anything failed.
#pragma once

#include <iostream>
#include <string>

namespace testing {

inline std::size_t failure_count = 0;
inline std::size_t check_count = 0;
inline std::string current_section = "<none>";
inline constexpr std::size_t reported_failure_limit = 40;

inline void section(const std::string& name) {
  current_section = name;
}

inline void check(bool condition, const std::string& description) {
  ++check_count;
  if (condition) {
    return;
  }
  ++failure_count;
  if (failure_count <= reported_failure_limit) {
    std::cout << "FAIL [" << current_section << "] " << description << '\n';
  } else if (failure_count == reported_failure_limit + 1) {
    std::cout << "... further failures suppressed\n";
  }
}

template <typename left_type, typename right_type>
void check_equal(const left_type& actual, const right_type& expected,
                 const std::string& description) {
  ++check_count;
  if (actual == expected) {
    return;
  }
  ++failure_count;
  if (failure_count <= reported_failure_limit) {
    std::cout << "FAIL [" << current_section << "] " << description << ": expected " << expected
              << ", got " << actual << '\n';
  } else if (failure_count == reported_failure_limit + 1) {
    std::cout << "... further failures suppressed\n";
  }
}

template <typename exception_type, typename callable>
void check_throws(callable action, const std::string& description) {
  ++check_count;
  try {
    action();
  } catch (const exception_type&) {
    return;
  } catch (const std::exception& raised) {
    ++failure_count;
    std::cout << "FAIL [" << current_section << "] " << description
              << ": threw the wrong type, message was \"" << raised.what() << "\"\n";
    return;
  }
  ++failure_count;
  std::cout << "FAIL [" << current_section << "] " << description << ": nothing was thrown\n";
}

template <typename callable>
void check_does_not_throw(callable action, const std::string& description) {
  ++check_count;
  try {
    action();
  } catch (const std::exception& raised) {
    ++failure_count;
    std::cout << "FAIL [" << current_section << "] " << description << ": threw \"" << raised.what()
              << "\"\n";
  }
}

// Counter-based splitmix64. Fixed seed, no std distribution, so a failing test reproduces
// exactly on any machine and any standard library.
class random_source {
 public:
  explicit random_source(std::uint64_t initial_state) : _state(initial_state) {}

  std::uint64_t next() {
    _state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t mixed = _state;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebULL;
    return mixed ^ (mixed >> 31);
  }

  std::size_t below(std::size_t bound) {
    return static_cast<std::size_t>(next() % bound);
  }

 private:
  std::uint64_t _state;
};

inline std::size_t summarize(const std::string& name) {
  if (failure_count == 0) {
    std::cout << name << ": all " << check_count << " checks passed\n";
    return 0;
  }
  std::cout << name << ": " << failure_count << " of " << check_count << " checks FAILED\n";
  return 1;
}

}  // namespace testing
