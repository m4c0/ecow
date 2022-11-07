#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class objc : public unit {
  std::unordered_set<std::string> m_frameworks{};

public:
  using unit::unit;

  void add_framework(const std::string &name) { m_frameworks.insert(name); }

  [[nodiscard]] virtual bool build(const std::string &flags = "") override {
    return !target_supports(impl::target::objective_c) || unit::build(flags);
  }

  [[nodiscard]] virtual strvec objects() const override {
    return target_supports(impl::target::objective_c) ? unit::objects()
                                                      : strvec{};
  }

  [[nodiscard]] virtual strset frameworks() const override {
    return target_supports(impl::target::objective_c) ? m_frameworks : strset{};
  }
};
} // namespace ecow
