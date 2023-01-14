#pragma once

#include "ecow.feat.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace ecow::impl {
class target;

static auto &target_stack() {
  static std::vector<target *> i{};
  return i;
}
static inline target *&current_target() { return target_stack().back(); }

class deco_target;
class target {
  friend class deco_target;

protected:
  [[nodiscard]] virtual std::string build_subfolder() const = 0;

  [[nodiscard]] static inline std::string default_clang() {
#ifdef __APPLE__
    return "/usr/local/opt/llvm/bin/clang++ ";
#elif WIN32
    return "clang++ ";
#else
    return "/home/linuxbrew/.linuxbrew/bin/clang++ ";
#endif
  }

public:
  target() { target_stack().push_back(this); }
  virtual ~target() { target_stack().pop_back(); }

  [[nodiscard]] virtual bool supports(features f) const { return false; }

  [[nodiscard]] virtual std::string cxx() const = 0;
  [[nodiscard]] virtual std::string ld() const = 0;

  [[nodiscard]] virtual std::string
  app_exe_name(const std::string &name) const = 0;

  [[nodiscard]] std::string build_folder() const {
    return "out/" + build_subfolder() + "/";
  }
  [[nodiscard]] auto build_path() const {
    return std::filesystem::path{build_folder()};
  }
  [[nodiscard]] virtual std::set<std::string> prebuilt_module_paths() const {
    std::set<std::string> res;
    res.insert(build_folder());
    return res;
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

class deco_target : public target {
  target *m_prev;

protected:
  [[nodiscard]] virtual std::string build_subfolder() const override {
    return m_prev->build_subfolder();
  }

public:
  deco_target() : target{}, m_prev{*(target_stack().end() - 2)} {}

  [[nodiscard]] virtual bool supports(features f) const override {
    return m_prev->supports(f);
  }

  [[nodiscard]] virtual std::string cxx() const override {
    return m_prev->cxx();
  }
  [[nodiscard]] virtual std::string ld() const override { return m_prev->ld(); }

  [[nodiscard]] virtual std::string
  app_exe_name(const std::string &name) const override {
    return m_prev->app_exe_name(name);
  }

  [[nodiscard]] virtual std::filesystem::path
  module_cache_path() const override {
    return m_prev->module_cache_path();
  }
  [[nodiscard]] virtual std::set<std::string>
  prebuilt_module_paths() const override {
    return m_prev->prebuilt_module_paths();
  }

  [[nodiscard]] virtual std::filesystem::path
  resource_path(const std::string &name) const override {
    return m_prev->resource_path(name);
  }
};
} // namespace ecow::impl
