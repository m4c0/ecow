#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class host_target : public target {
protected:
  [[nodiscard]] std::string build_subfolder() const override { return "linux"; }

public:
  [[nodiscard]] std::string cxxflags() const override { return ""; }
  [[nodiscard]] std::string ldflags() const override { return ""; }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name;
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    return f == host;
  }
};
} // namespace ecow::impl
