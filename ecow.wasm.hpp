#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class wasm_target : public target {
  std::string m_cxx;
  std::string m_ld;

  [[nodiscard]] static inline auto sdk_path() {
    const auto env = std::getenv("WASI_SDK_ROOT");
    if (!env)
      throw std::runtime_error("WASI_SDK_ROOT undefined");

    return std::filesystem::path{env};
  }

protected:
  [[nodiscard]] std::string build_subfolder() const override { return "wasm"; }

public:
  wasm_target() {
    constexpr const auto target = " -target wasm32-wasi ";

    auto sdk = sdk_path();
    auto sysroot = sdk / "share" / "wasi-sysroot";
    auto clang = sdk / "bin" / "clang++";

    m_cxx = default_clang() + target + " --sysroot " + sysroot.string();
    m_ld = clang.string() + target;
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
