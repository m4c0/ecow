#pragma once
#include <string>

namespace ecow::impl {
class android_target : public target {
  static constexpr const auto llvm =
      "F:/Apps/Android/ndk-bundle/toolchains/llvm/prebuilt/windows-x86_64/";

  std::string m_ld;
  std::string m_extra_cflags;
  std::string m_target;

protected:
  [[nodiscard]] std::string build_subfolder() const override {
    return m_target;
  }

public:
  explicit android_target(const std::string &tgt) : m_target{tgt} {
    m_ld = std::string{llvm} + "bin/clang++";
    m_extra_cflags =
        " -fPIC -DANDROID -fdata-sections -ffunction-sections "
        "-funwind-tables "
        "-fstack-protector-strong -no-canonical-prefixes --target=" +
        tgt + " --sysroot " + llvm + "sysroot";
  }

  [[nodiscard]] std::string cxx() const override {
    return "clang++ " + m_extra_cflags;
  }
  [[nodiscard]] std::string ld() const override {
    return m_ld + " " + m_extra_cflags + " -shared -static-libstdc++";
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return "lib" + name + ".so";
  }
};
} // namespace ecow::impl
