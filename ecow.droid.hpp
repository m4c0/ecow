#pragma once

#include "ecow.target.hpp"

#include <filesystem>
#include <string>

namespace ecow::impl {
class android_target : public target {
  std::string m_target;

  [[nodiscard]] static inline auto sdk_path() {
    const auto env = std::getenv("ANDROID_SDK_ROOT");
    if (!env)
      throw std::runtime_error("ANDROID_SDK_ROOT undefined");

    return std::filesystem::path{env};
  }
  [[nodiscard]] static inline auto find_latest_ndk() {
    const auto sdk = sdk_path();

    const auto ndk_bundle = sdk / "ndk-bundle";
    if (std::filesystem::is_directory(ndk_bundle)) {
      return ndk_bundle;
    }

    const auto ndk_root = sdk / "ndk";
    if (!std::filesystem::is_directory(ndk_root) ||
        std::filesystem::is_empty(ndk_root)) {
      throw std::runtime_error("No NDK installed");
    }
    const auto ndk_iter = std::filesystem::directory_iterator{ndk_root};
    std::filesystem::directory_entry max{};
    for (auto x : ndk_iter) {
      if (x > max)
        max = x;
    }
    return max.path();
  }

  [[nodiscard]] static inline auto find_llvm() {
    const auto ndk = find_latest_ndk();
    const auto prebuilt = ndk / "toolchains" / "llvm" / "prebuilt";
    if (!std::filesystem::is_directory(prebuilt) ||
        std::filesystem::is_empty(prebuilt)) {
      throw std::runtime_error("LLVM not found in NDK");
    }
    return (*std::filesystem::directory_iterator{prebuilt}).path();
  }

protected:
  [[nodiscard]] std::string build_subfolder() const override {
    return m_target;
  }

public:
  explicit android_target(const std::string &tgt) : m_target{tgt} {
    const auto llvm = find_llvm().string();

    add_flags("-fdata-sections", "-ffunction-sections", "-funwind-tables",
              "-fstack-protector-strong", "-no-canonical-prefixes",
              " --target=" + tgt, "--sysroot", llvm + "/sysroot");

    add_ldflags("-shared", "-static-libstdc++", "-Wl,-Bsymbolic",
                "-fuse-ld=lld", "-Wl,--no-undefined", "-resource-dir",
                llvm + "/lib64/clang/*");
  }

  [[nodiscard]] std::string triple() const override { return m_target; }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return "lib" + name + ".so";
  }
  [[nodiscard]] std::string exe_name(const std::string &name) const override {
    return name;
  }

  [[nodiscard]] bool supports(features f) const override {
    switch (f) {
    case application:
    case android_ndk:
    case native:
      return true;
    default:
      return false;
    }
  }
};
} // namespace ecow::impl
