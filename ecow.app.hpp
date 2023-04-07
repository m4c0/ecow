#pragma once

#include "ecow.exe.hpp"

#include <fstream>
#include <numeric>

namespace ecow {
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

class app : public exe {
protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target()->app_exe_name(name());
  }

  void build_self() const override {
    if (target_supports(webassembly))
      wasm_bundler::build_wasm(name(), *this);

    exe::build_self();

    const auto res_fld = impl::current_target()->resource_path(name());
    for (const auto &res : resources()) {
      impl::run_copy(res, res_fld / res.filename());
    }
  };

public:
  explicit app(const std::string &name) : exe{name} {}
};
} // namespace ecow
