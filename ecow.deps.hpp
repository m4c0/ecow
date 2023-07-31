#pragma once
#include <map>
#include <set>
#include <string>

namespace ecow::deps {
static std::map<std::string, std::set<std::string>> mappings{};

[[nodiscard]] const auto &of(const std::string &n) {
  static std::set<std::string> empty{};

  return mappings.contains(n) ? mappings[n] : empty;
}
} // namespace ecow::deps
