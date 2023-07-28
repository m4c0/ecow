#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class host_target : public target {
protected:
  [[nodiscard]] std::string build_subfolder() const override { return "win32"; }

public:
  explicit host_target() { add_cxxflags("-fno-ms-compatibility"); }

  [[nodiscard]] std::string triple() const override {
    return "x86_64-pc-windows-msvc";
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name + ".exe";
  }
  [[nodiscard]] std::string exe_name(const std::string &name) const override {
    return name + ".exe";
  }

  [[nodiscard]] virtual std::filesystem::path
  module_cache_path() const override {
    std::filesystem::path home{std::getenv("LOCALAPPDATA")};
    return home / "ecow" / "cache" / build_subfolder();
  }

  [[nodiscard]] bool supports(features f) const override {
    switch (f) {
    case application:
    case host:
    case native:
    case posix:
    case windows_api:
      return true;
    default:
      return false;
    }
  }
};

[[nodiscard]] static inline std::string popen(const std::string &cmd) {
  auto f = ::_popen(cmd.c_str(), "r");
  char buf[1024];
  std::string res{fgets(buf, 1024, f)};
  if (res.back() == '\n')
    res.pop_back();
  return res;
}

auto clang_dir() {
  return std::filesystem::path{impl::popen("where clang.exe")}
      .parent_path()
      .parent_path();
}
} // namespace ecow::impl
