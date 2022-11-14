#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "ecow.target.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <span>

namespace ecow::impl {
struct clang_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};

static auto &current_target() {
  static std::unique_ptr<target> i{};
  return i;
}

[[nodiscard]] static inline auto last_write_time(auto path) {
  if (std::filesystem::exists(path))
    return std::filesystem::last_write_time(path);
  return std::filesystem::file_time_type::min();
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

static inline void run_clang(const std::string &args, const std::string &from,
                             const std::string &to) {
  const auto ftime = last_write_time(from);
  const auto ttime = last_write_time(to);
  if (ttime > ftime)
    return;

  const auto bfld = current_target()->build_folder();

  std::cerr << "compiling " << to << std::endl;
  const auto cmd = cxx() +
                   " -fmodules -fobjc-arc -std=c++20"
                   " -fmodules-cache-path=" +
                   bfld + " -fprebuilt-module-path=" + bfld + " " + args + " " +
                   from + " -o " + to;
  if (std::system(cmd.c_str()))
    throw clang_failed{cmd};
}
} // namespace ecow::impl
