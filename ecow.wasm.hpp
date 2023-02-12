#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class wasm_target : public target {
  std::string m_cxxflags;
  std::string m_ld;

  [[nodiscard]] static inline auto sysroot_path() {
    const auto env = std::getenv("WASI_SYSROOT");
    if (env)
      return std::filesystem::path{env};

    return std::filesystem::current_path().parent_path() / "wasi-sysroot";
  }

protected:
  [[nodiscard]] std::string build_subfolder() const override { return "wasm"; }

public:
  wasm_target() {
    constexpr const auto target = " -target wasm32-wasi ";

    auto sysroot = sysroot_path();
    if (!std::filesystem::exists(sysroot))
      throw std::runtime_error("Invalid wasi sysroot");

    auto flags = std::string{target} + " --sysroot " + sysroot.string();

    m_cxxflags =
        flags + " -D_LIBCPP_SETJMP_H -D_LIBCPP_CSIGNAL -fno-exceptions";
    m_ld = cxx() + flags + " -resource-dir " + sysroot.string();
  }

  [[nodiscard]] std::string cxxflags() const override { return m_cxxflags; }
  [[nodiscard]] std::string ld() const override { return m_ld; }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name + ".wasm";
  }

  [[nodiscard]] bool supports(features f) const override {
    switch (f) {
    case webassembly:
      return true;
    default:
      return false;
    }
  }
};
} // namespace ecow::impl
