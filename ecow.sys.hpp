#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class sys : public unit {
public:
  using unit::unit;

  [[nodiscard]] bool build(args_t args) override {
    return std::system(name().c_str()) == 0;
  }
  void clean(args_t args) override {}

  [[nodiscard]] strvec objects() const override { return strvec(); }
};
} // namespace ecow
