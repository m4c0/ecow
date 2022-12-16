#pragma once

#include "ecow.feat.hpp"

#include <filesystem>
#include <string>

namespace ecow::impl {
class target {
protected:
  [[nodiscard]] virtual std::string build_subfolder() const = 0;

  [[nodiscard]] static inline std::string default_clang() {
#ifdef __APPLE__
    return "/usr/local/opt/llvm/bin/clang++ ";
#elif WIN32
    return "clang++ ";
#else
    return "clang++-15 ";
#endif
  }

public:
  virtual ~target() = default;

  [[nodiscard]] virtual bool supports(features f) const { return false; }

  [[nodiscard]] virtual std::string cxx() const = 0;
  [[nodiscard]] virtual std::string ld() const = 0;

  [[nodiscard]] virtual std::string
  app_exe_name(const std::string &name) const = 0;

  [[nodiscard]] std::string build_folder() const {
    return "out/" + build_subfolder() + "/";
  }

  [[nodiscard]] virtual std::filesystem::path module_cache_path() const {
    std::filesystem::path home{std::getenv("HOME")};
    return home / ".ecow" / "cache" / build_subfolder();
  }

  [[nodiscard]] virtual std::filesystem::path
  resource_path(const std::string &name) const {
    return build_folder();
  }
};
} // namespace ecow::impl
