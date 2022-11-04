#pragma once
#include <string>

namespace ecow::impl {
class target {
  [[nodiscard]] std::string cxx() const {
    return "clang++ -fno-ms-compatibility";
  }
  [[nodiscard]] std::string ld() const { return "clang++"; }
};
} // namespace ecow::impl
