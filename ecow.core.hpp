#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <iostream>
#include <memory>
#include <span>

namespace ecow::impl {
class target {
protected:
  [[nodiscard]] virtual std::string build_subfolder() const = 0;

public:
  virtual ~target() = default;

  [[nodiscard]] virtual std::string cxx() const = 0;
  [[nodiscard]] virtual std::string ld() const = 0;
  [[nodiscard]] virtual std::string
  app_exe_name(const std::string &name) const = 0;
  [[nodiscard]] std::string build_folder() const {
    return "out/" + build_subfolder() + "/";
  }
};
static auto &current_target() {
  static std::unique_ptr<target> i{};
  return i;
}

[[nodiscard]] static inline auto last_write_time(auto path) {
  if (std::filesystem::exists(path))
    return std::filesystem::last_write_time(path);
  return std::filesystem::file_time_type{};
}

[[nodiscard]] static inline std::string cxx() {
  if (const char *exe = std::getenv("CXX")) {
    return exe;
  }
  return current_target()->cxx();
}
[[nodiscard]] static inline std::string ld() {
  if (const char *exe = std::getenv("LD")) {
    return exe;
  }
  return current_target()->ld();
}

[[nodiscard]] static inline bool run_clang(const std::string &args,
                                           const std::string &from,
                                           const std::string &to) {
  const auto ftime = last_write_time(from);
  const auto ttime = last_write_time(to);
  if (ttime > ftime)
    return true;

  std::cerr << "compiling " << to << std::endl;
  const auto cmd = cxx() + " -fobjc-arc -std=c++20 -fprebuilt-module-path=" +
                   impl::current_target()->build_folder() + " " + args + " " +
                   from + " -o " + to;
  return std::system(cmd.c_str()) == 0;
}
} // namespace ecow::impl
