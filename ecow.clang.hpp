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
    for (const auto &path : current_target()->prebuilt_module_paths())
      add_arg("-fprebuilt-module-path=" + path);
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
} // namespace ecow::impl
