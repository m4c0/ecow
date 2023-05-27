#pragma once

#include "ecow.target.hpp"

#include <string>

namespace ecow::impl {
class wasm_bundler {
  static void build_fn(std::ofstream &o, std::istream &vv) {
    bool first = true;
    for (std::string line; std::getline(vv, line);) {
      if (first) {
        first = false;
      } else {
        o << "\n    ";
      }
      o << line;
    }
  }
  static void build_fn(std::ofstream &o, const std::string &v) {
    std::istringstream i{v};
    build_fn(o, i);
    o << ",";
  }
  static void build_env(std::ofstream &o, const unit &u) {
    std::map<std::string, std::string> env;
    u.visit(wasm_env, env);
    for (auto &[k, v] : env) {
      o << "\n    " << k << ": ";
      build_fn(o, v);
    }
  }
  static void build_inits(std::ofstream &o, const unit &u) {
    std::map<std::string, std::string> env;
    u.visit(wasm_setup, env);
    for (auto &[k, v] : env) {
      o << "\n    ";
      build_fn(o, v);
    }
  }

public:
  static void build_wasm(const std::string &name, const unit &u) {
    const auto fdir =
        std::filesystem::path{impl::current_target()->build_folder()};
    const auto fname = (fdir / name).replace_extension("js");

    std::cerr << "javascripting " << fname.string() << std::endl;
    std::ofstream o{fname};

    o << R"(function ecow(options) {
  var ecow_buffer;
  var ecow_exports;
  var ecow_globals = {};

  var name = ")" +
             name + R"(";
  var inits = [)";
    build_inits(o, u);
    o << R"(
  ];
  var env = {)";
    build_env(o, u);
    o << R"(
  };
)";
    const auto me = std::filesystem::path{__FILE__}.parent_path();
    o << std::ifstream(me / "ecow.js").rdbuf();
    o << "}";
  }
};

class wasm_target : public target {
  [[nodiscard]] static inline auto sysroot_path() {
    const auto env = std::getenv("WASI_SYSROOT");
    if (env)
      return std::filesystem::path{env};

    return std::filesystem::current_path().parent_path() / "wasi-sysroot";
  }

protected:
  [[nodiscard]] std::string build_subfolder() const override { return "wasm"; }

public:
  wasm_target() {
    auto sysroot = sysroot_path();
    if (!std::filesystem::exists(sysroot))
      throw std::runtime_error("Invalid wasi sysroot");

    add_flags("-target", "wasm32-wasi", "--sysroot", sysroot.string());
    add_cxxflags("-D_LIBCPP_SETJMP_H", "-D_LIBCPP_CSIGNAL", "-fno-exceptions");
    add_ldflags("-resource-dir", sysroot.string(), "-mexec-model=reactor",
                "-flto", "-Wl,--lto-O3");
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    return name + ".wasm";
  }
  [[nodiscard]] std::string exe_name(const std::string &name) const override {
    return name + ".wasm";
  }

  [[nodiscard]] bool supports(features f) const override {
    switch (f) {
    case application:
    case wasm_env:
    case wasm_setup:
    case webassembly:
      return true;
    default:
      return false;
    }
  }

  void bundle(const std::string &name, const unit &u) const override {
    wasm_bundler::build_wasm(name, u);
  }
};
} // namespace ecow::impl
