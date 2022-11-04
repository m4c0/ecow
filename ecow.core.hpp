#pragma once

#include <filesystem>
#include <iostream>
#include <span>

namespace ecow::impl {

[[nodiscard]] static inline auto last_write_time(auto path) {
  if (std::filesystem::exists(path))
    return std::filesystem::last_write_time(path);
  return std::filesystem::file_time_type{};
}

[[nodiscard]] static inline std::string cxx() {
  if (const char *exe = std::getenv("CXX")) {
    return exe;
  }
#ifdef __APPLE__
  return "/usr/local/opt/llvm/bin/clang++ -isysroot "
         "/Applications/Xcode.app/Contents/Developer/Platforms/"
         "MacOSX.platform/Developer/SDKs/MacOSX.sdk";
#elif _WIN32
  return "clang++ -fno-ms-compatibility";
#else
  return "clang++";
#endif
}
[[nodiscard]] static inline std::string ld() {
  if (const char *exe = std::getenv("LD")) {
    return exe;
  }
  return "clang++";
}

[[nodiscard]] static inline std::string ext_of(const std::string &who) {
  if (std::filesystem::exists(who + ".cpp")) {
    return ".cpp";
  }
  if (std::filesystem::exists(who + ".mm")) {
    return ".mm";
  }
  return "";
}

[[nodiscard]] static inline bool run_clang(const std::string &args,
                                           const std::string &from,
                                           const std::string &to) {
  const auto ftime = last_write_time(from);
  const auto ttime = last_write_time(to);
  if (ttime > ftime)
    return true;

  std::cerr << "compiling " << to << std::endl;
  const auto cmd = cxx() + " -fobjc-arc -std=c++20 -fprebuilt-module-path=. " +
                   args + " " + from + " -o " + to;
  return std::system(cmd.c_str()) == 0;
}

static inline void remove(std::string name) {
  if (std::filesystem::exists(name)) {
    std::cerr << "removing " << name << std::endl;
    std::filesystem::remove(name);
  }
}
} // namespace ecow::impl
