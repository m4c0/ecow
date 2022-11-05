#pragma once
#include <string>

namespace ecow::impl {
class target {
  [[nodiscard]] std::string cxx() const {
    return "clang++ -fno-ms-compatibility";
  }
  [[nodiscard]] std::string ld() const { return "clang++"; }

  [[nodiscard]] std::string app_exe_name(const std::string &name) const {
    return name + ".exe";
  }

  [[nodiscard]] std::string build_folder() const { return "out/"; }
};
} // namespace ecow::impl
