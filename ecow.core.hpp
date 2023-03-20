#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "ecow.target.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <span>
#include <sstream>
#include <string>

namespace ecow::impl {
[[nodiscard]] static inline std::string c() {
  // NOTE: this are host-dependent, not target-dependent
#ifdef __APPLE__
  return "/usr/local/opt/llvm/bin/clang ";
#elif defined(_WIN32)
  return "clang ";
#else
  return "/home/linuxbrew/.linuxbrew/bin/clang ";
#endif
}
[[nodiscard]] static inline std::string cxx() {
  // NOTE: this are host-dependent, not target-dependent
#ifdef __APPLE__
  return "/usr/local/opt/llvm/bin/clang++ ";
#elif defined(_WIN32)
  return "clang++ ";
#else
  return "/home/linuxbrew/.linuxbrew/bin/clang++ ";
#endif
}

[[nodiscard]] static inline auto last_write_time(auto path) {
  if (std::filesystem::exists(path))
    return std::filesystem::last_write_time(path);
  return std::filesystem::file_time_type::min();
}
[[nodiscard]] static inline bool must_recompile(const auto &from,
                                                const auto &to) {
  const auto ftime = impl::last_write_time(from);
  const auto ttime = impl::last_write_time(to);
  return ftime > ttime;
}
[[nodiscard]] static inline bool
must_recompile(const auto &depfn, const auto &from, const auto &to) {
  if (!std::filesystem::exists(to))
    return true;

  std::ifstream deps{depfn};
  if (!deps)
    return true;

  if (must_recompile(from, to))
    return true;

  const auto ttime = impl::last_write_time(to);
  std::string line;
  while (std::getline(deps, line)) {
    if (!line.starts_with("  "))
      continue;

    line = line.substr(2);
    if (line.ends_with(" \\"))
      line.resize(line.size() - 2);

    const auto deptime = last_write_time(line);
    if (deptime > ttime)
      return true;
  }

  return false;
}

static inline auto find_actual_file(const std::filesystem::path &from) {
  if (std::filesystem::exists(from))
    return from;

  auto actual_from = "out" / from;
  if (std::filesystem::exists(actual_from))
    return from;

  throw std::runtime_error(std::string{"missing file: "} + from.string());
}
static inline void run_copy(const std::filesystem::path &from,
                            const std::filesystem::path &to) {
  auto actual_from = find_actual_file(from);
  if (!must_recompile(actual_from, to))
    return;

  std::cout << "copying " << to.string() << "\n";
  std::filesystem::copy(actual_from, to,
                        std::filesystem::copy_options::update_existing);
}
} // namespace ecow::impl
