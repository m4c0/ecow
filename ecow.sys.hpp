#pragma once

#include "ecow.unit.hpp"

namespace ecow {
struct command_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};
class sys : public unit {
public:
  explicit sys(const std::string &path) : unit{path} {}
  explicit sys(std::filesystem::path path)
      : unit{path.make_preferred().string()} {}

  void build() override {
    if (std::system(name().c_str()) != 0)
      throw command_failed(name());
  }

  [[nodiscard]] pathset objects() const override { return {}; }
};
} // namespace ecow
