#pragma once
#include <string>

namespace ecow::impl {
class host_target : public target {
protected:
  [[nodiscard]] std::string build_subfolder() const override { return "linux"; }

public:
  [[nodiscard]] std::string cxx() const override { return "clang++"; }
  [[nodiscard]] std::string ld() const override { return "clang++"; }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name;
  }
};
} // namespace ecow::impl
