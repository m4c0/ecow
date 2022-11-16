#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class wasm_target : public target {
  std::string m_cxx;
  std::string m_ld;

  [[nodiscard]] static inline auto sysroot_path() {
    const auto env = std::getenv("WASI_SYSROOT");
    if (!env)
      throw std::runtime_error("WASI_SYSROOT undefined");

    return std::filesystem::path{env};
  }

protected:
  [[nodiscard]] std::string build_subfolder() const override { return "wasm"; }

public:
  wasm_target() {
    constexpr const auto target = " -target wasm32-wasi ";

    auto sysroot = sysroot_path();
    auto clang = default_clang() + target + " --sysroot " + sysroot.string();

    m_cxx = clang + " -D_LIBCPP_SETJMP_H -D_LIBCPP_CSIGNAL -fno-exceptions";
    m_ld = clang + " -resource-dir " + sysroot.string();
  }

  [[nodiscard]] std::string cxx() const override { return m_cxx; }
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
