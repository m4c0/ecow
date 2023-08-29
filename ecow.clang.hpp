#pragma once

#include "ecow.core.hpp"
#include "ecow.llvm.hpp"
#include "ecow.target.hpp"

#include <filesystem>
#include <set>
#include <string>

namespace ecow::impl {
struct clang_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};

std::filesystem::path clang_dir();

class clang {
  std::set<std::string> m_args{};
  std::string m_from;
  std::string m_to;
  bool m_cpp{};
  bool m_with_deps{false};

  static void escape(std::ostream &o, std::filesystem::path p) {
    for (auto c : p.make_preferred().string()) {
      if (c == '\\')
        o << "\\";
      o << c;
    }
  }

  [[nodiscard]] auto depfile() const { return m_to + ".deps"; }

  auto clang_exe() const {
    return clang_dir() / "bin" / (m_cpp ? "clang++" : "clang");
  }

  auto full_cmd() const {
    std::stringstream o;
    o << (m_cpp ? "clang++" : "clang");
    for (const auto &a : current_target()->cxxflags())
      o << ' ' << a;
    for (const auto &a : m_args)
      o << ' ' << '\"' << a << '\"';
    o << ' ' << m_from;
    o << ' ' << "-o" << ' ';
    escape(o, std::filesystem::current_path() / m_to);

    return o.str();
  }

  auto llvm_in() const {
    llvm::input r{
        .from = m_from,
        .to =
            (std::filesystem::current_path() / m_to).make_preferred().string(),
        .clang_exe = clang_exe().string(),
        .triple = impl::current_target()->triple(),
    };
    r.cmd_line.push_back(r.clang_exe);
    for (const auto &a : current_target()->cxxflags())
      r.cmd_line.push_back(a);
    for (const auto &a : m_args)
      r.cmd_line.push_back(a);
    r.cmd_line.push_back(r.from);
    r.cmd_line.push_back("-o");
    r.cmd_line.push_back(r.to);
    return r;
  }

  [[nodiscard]] bool run_with_powa() const { return llvm::compile(llvm_in()); }
  [[nodiscard]] bool run_with_driver() {
    return 0 == std::system(full_cmd().c_str());
  }

  void run(auto &&fn, bool force) {
    if (!force && !must_recompile())
      return;

    std::cerr << "compiling " << m_to << std::endl;
    if (!(this->*fn)())
      throw clang_failed{full_cmd()};
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

    m_cpp = (fext != ".c" && fext != ".m");

    if (fext != ".c" && fext != ".m" && fext != ".pcm") {
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

  [[nodiscard]] bool must_recompile() {
    if (m_with_deps && impl::must_recompile(depfile(), m_from, m_to))
      return true;

    if (!m_with_deps && impl::must_recompile(m_from, m_to))
      return true;

    return false;
  }
  void run(bool force = false) { run(&clang::run_with_driver, force); }
  void powa_run() { run(&clang::run_with_powa, false); }

#ifndef ECOW_META_BUILD
  [[nodiscard]] std::set<std::string> generate_deps() {
    auto [deps, success] =
        llvm::find_deps(llvm_in(), depfile(), must_recompile());
    if (!success)
      throw clang_failed{full_cmd()};

    return deps;
  }
#else
  [[nodiscard]] std::set<std::string> generate_deps() { return {}; }
#endif
};
} // namespace ecow::impl
