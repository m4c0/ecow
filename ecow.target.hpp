#pragma once

#include <string>

namespace ecow::impl {
class target {
protected:
  [[nodiscard]] virtual std::string build_subfolder() const = 0;

public:
  enum features {
    android_ndk,
    cocoa,
    objective_c,
    uikit,
    windows_api,
    webassembly,
  };

  virtual ~target() = default;

  [[nodiscard]] virtual bool supports(features f) const { return false; }

  [[nodiscard]] virtual std::string cxx() const = 0;
  [[nodiscard]] virtual std::string ld() const = 0;
  [[nodiscard]] virtual std::string
  app_exe_name(const std::string &name) const = 0;
  [[nodiscard]] std::string build_folder() const {
    return "out/" + build_subfolder() + "/";
  }
};
} // namespace ecow::impl
