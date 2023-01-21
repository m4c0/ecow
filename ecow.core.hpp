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
struct clang_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};

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

class clang {
  std::set<std::string> m_args{};
  std::string m_from;
  std::string m_to;
  bool m_with_deps{false};

  [[nodiscard]] auto depfile() const { return m_to + ".deps"; }

public:
  clang(const std::string &from, const std::string &to)
      : m_from{from}, m_to{to} {
    const auto fext = std::filesystem::path{from}.extension();
    if (fext == ".mm") {
      add_arg("-fobjc-arc");
    } else {
      add_arg("-std=c++2b");
    }

    add_arg("-O3");
    add_arg("-fmodules");
    add_arg("-fmodules-cache-path=" +
            current_target()->module_cache_path().string());
    for (const auto &path : current_target()->prebuilt_module_paths())
      add_arg("-fprebuilt-module-path=" + path);
  }

  clang &add_arg(const std::string &a) {
    m_args.insert(a);
    return *this;
  }
  clang &with_deps() {
    add_arg("-MMD");
    add_arg("-MF " + depfile());
    m_with_deps = true;
    return *this;
  }

  void run() {
    if (m_with_deps && !impl::must_recompile(depfile(), m_from, m_to))
      return;

    if (!m_with_deps && !impl::must_recompile(m_from, m_to))
      return;

    std::cerr << "compiling " << m_to << std::endl;

    std::stringstream cbuf;
    cbuf << cxx();
    for (const auto &a : m_args)
      cbuf << " " << a;
    cbuf << " " << m_from;
    cbuf << " -o " << m_to;
    cbuf << " -MJ " << m_to << ".json";

    auto cmd = cbuf.str();
    if (std::system(cmd.c_str()))
      throw clang_failed{cmd};
  }
};

static inline void run_copy(const std::filesystem::path &from,
                            const std::filesystem::path &to) {
  if (!must_recompile(from, to))
    return;

  std::cout << "copying " << to.string() << "\n";
  std::filesystem::copy(from, to,
                        std::filesystem::copy_options::update_existing);
}
} // namespace ecow::impl
