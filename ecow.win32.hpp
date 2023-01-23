#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class host_target : public target {
protected:
  [[nodiscard]] std::string build_subfolder() const override { return "win32"; }

public:
  [[nodiscard]] std::string cxx() const override {
    return "clang++ -fno-ms-compatibility";
  }
  [[nodiscard]] std::string ld() const override { return "clang++"; }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name + ".exe";
  }

  [[nodiscard]] virtual std::filesystem::path
  module_cache_path() const override {
    std::filesystem::path home{std::getenv("LOCALAPPDATA")};
    return home / "ecow" / "cache" / build_subfolder();
  }

  [[nodiscard]] bool supports(features f) const override {
    switch (f) {
    case posix:
    case windows_api:
      return true;
    default:
      return false;
    }
  }
};
} // namespace ecow::impl
