#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class objc : public unit {
public:
  explicit objc(const std::string &name) : unit{name + ".mm"} {}

  void add_framework(const std::string &name) {
    add_link_flag("-framework " + name);
  }

  void build() override {
    if (target_supports(objective_c))
      unit::build();
  }

  [[nodiscard]] virtual strvec objects() const override {
    return target_supports(objective_c) ? unit::objects() : strvec{};
  }

  [[nodiscard]] virtual strset link_flags() const override {
    return target_supports(objective_c) ? unit::link_flags() : strset{};
  }
};
} // namespace ecow
