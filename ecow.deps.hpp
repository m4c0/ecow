#pragma once
#include <map>
#include <set>
#include <string>

namespace ecow::deps {
static std::map<std::string, std::set<std::string>> mappings{};

void add(const std::string &from, const std::string &to) {
  mappings[from].insert(to);
}

[[nodiscard]] bool has(const std::string &n) { return mappings.contains(n); }

[[nodiscard]] const auto &of(const std::string &n) {
  static std::set<std::string> empty{};
  return has(n) ? mappings[n] : empty;
}
} // namespace ecow::deps
