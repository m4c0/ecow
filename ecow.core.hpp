#pragma once

#ifdef __APPLE__
#include "ecow.apple.hpp"
#elif _WIN32
#include "ecow.win32.hpp"
#endif

#include <filesystem>
#include <iostream>
#include <span>

namespace ecow::impl {
static target &current_target() {
  static target i{};
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
  return current_target().cxx();
}
[[nodiscard]] static inline std::string ld() {
  if (const char *exe = std::getenv("LD")) {
    return exe;
  }
  return current_target().ld();
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
                   impl::current_target().build_folder() + " " + args + " " +
                   from + " -o " + to;
  return std::system(cmd.c_str()) == 0;
}

static inline void remove(std::string name) {
  if (std::filesystem::exists(name)) {
    std::cerr << "removing " << name << std::endl;
    std::filesystem::remove(name);
  }
}
} // namespace ecow::impl
