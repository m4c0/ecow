#pragma once

#include "ecow.core.hpp"
#include "ecow.target.hpp"

#include <filesystem>
#include <set>
#include <string>

namespace ecow::impl {
struct clang_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};

class clang {
  std::set<std::string> m_args{};
  std::string m_compiler;
  std::string m_from;
  std::string m_to;
  bool m_with_deps{false};

  static void escape(std::ostream &o, std::filesystem::path p) {
    for (auto c : p.make_preferred().string()) {
      if (c == '\\')
        o << "\\";
      o << c;
    }
  }

  [[nodiscard]] auto depfile() const { return m_to + ".deps"; }

  void arguments(std::ostream &o, std::string_view sep) const {
    o << m_compiler;
    for (const auto &a : current_target()->cxxflags())
      o << sep << a;
    for (const auto &a : m_args)
      o << sep << a;
    o << sep << m_from;
    o << sep << "-o" << sep;
    escape(o, std::filesystem::current_path() / m_to);
  }

  void really_run() {
    std::stringstream cbuf;
    arguments(cbuf, " ");

    auto cmd = cbuf.str();
    if (std::system(cmd.c_str()))
      throw clang_failed{cmd};
  }

public:
  clang(const std::string &from, const std::string &to)
      : m_from{from}, m_to{to} {
    const auto fext = std::filesystem::path{from}.extension();
    if ((fext == ".mm") || (fext == ".m")) {
      add_arg("-fobjc-arc");
    } else if (fext != ".c") {
#ifdef ECOW_META_BUILD
      // Until this goes "live" in brew, etc
      // https://github.com/llvm/llvm-project/issues/59784
      add_arg("-std=c++20");
#else
      add_arg("-std=c++2b");
#endif
    }

    auto *xtra_cflags = std::getenv("ECOW_CFLAGS");
    if (xtra_cflags != nullptr) {
      add_arg(xtra_cflags);
    } else {
      add_arg("-O3");
    }
    // TODO: use -gdwarf on windows
    if (std::getenv("ECOW_DEBUG")) {
      add_arg("-g");
    }

    if (fext == ".c" || fext == ".m") {
      m_compiler = c();
    } else if (fext == ".pcm") {
      m_compiler = cxx();
    } else {
      m_compiler = cxx();

      for (const auto &path : current_target()->prebuilt_module_paths())
        add_arg("-fprebuilt-module-path=" + path);
    }
  }

  clang &add_include_dirs(const auto &incs) {
    for (auto &i : incs) {
      add_arg("-I" + i);
    }
    return *this;
  }
  clang &add_arg(const std::string &a) {
    m_args.insert(a);
    return *this;
  }
  clang &with_deps() {
    add_arg("-MMD");
    add_arg("-MF" + depfile());
    m_with_deps = true;
    return *this;
  }

  void run() {
    if (m_with_deps && !impl::must_recompile(depfile(), m_from, m_to))
      return;

    if (!m_with_deps && !impl::must_recompile(m_from, m_to))
      return;

    std::cerr << "compiling " << m_to << std::endl;
    really_run();
  }

  void create_cdb(std::ostream &o) const {
    const auto dir = std::filesystem::current_path();
    o << R"({ "directory": ")";
    escape(o, dir);
    o << R"(", "file": ")";
    escape(o, dir / m_from);
    o << R"(", "output": ")";
    escape(o, dir / m_to);
    o << R"(", "arguments": [")";
    arguments(o, R"(", ")");
    o << R"("]},)"
      << "\n";
  }
};
} // namespace ecow::impl
