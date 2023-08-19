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
  deps >> line;
  while (deps) {
    deps >> line;
    if (line == "\\")
      continue;

    const auto deptime = last_write_time(line);
    if (deptime > ttime)
      return true;
  }

  return false;
}

static inline auto find_actual_file(const std::filesystem::path &from) {
  if (std::filesystem::exists(from))
    return from;

  auto actual_from = from.parent_path() / "out" / from.filename();
  if (std::filesystem::exists(actual_from))
    return actual_from;

  actual_from = from.parent_path() / impl::current_target()->build_path() /
                from.filename();
  if (std::filesystem::exists(actual_from))
    return actual_from;

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
