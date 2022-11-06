#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class sys : public unit {
public:
  explicit sys(const std::string &path) : unit{path} {}
  explicit sys(std::filesystem::path path)
      : unit{path.make_preferred().string()} {}

  [[nodiscard]] bool build(const std::string &flags = "") override {
    return std::system(name().c_str()) == 0;
  }

  [[nodiscard]] strvec objects() const override { return strvec(); }
};
} // namespace ecow
