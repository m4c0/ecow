#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class objc : public unit {
protected:
  void build_self() const override {
    if (target_supports(objective_c))
      unit::build_self();
  }
  [[nodiscard]] virtual pathset self_objects() const override {
    return target_supports(objective_c) ? unit::self_objects() : pathset{};
  }

public:
  explicit objc(const std::string &name) : unit{name + ".mm"} {}

  void add_framework(const std::string &name) {
    add_link_flag("-framework " + name);
  }

  [[nodiscard]] virtual strset link_flags() const override {
    return target_supports(objective_c) ? unit::link_flags() : strset{};
  }
};
} // namespace ecow
