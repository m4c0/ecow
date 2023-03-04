#pragma once

#include "ecow.exe.hpp"

#include <fstream>
#include <numeric>

namespace ecow {
class app : public exe {
  std::vector<std::string> m_resources;

  [[nodiscard]] auto undefs_path() const noexcept {
    const auto fdir = impl::current_target()->build_path();
    return (fdir / exe_name()).replace_extension("syms");
  }

  void build_fn(std::ofstream &o, std::istream &vv) const {
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
  void build_fn(std::ofstream &o, const std::string &v) const {
    std::istringstream i{v};
    build_fn(o, i);
    o << ",";
  }
  void build_env(std::ofstream &o) const {
    strmap env;
    visit(wasm_env, env);
    for (auto &[k, v] : env) {
      o << "\n    " << k << ": ";
      build_fn(o, v);
    }
  }
  void build_inits(std::ofstream &o) const {
    strmap env;
    visit(wasm_setup, env);
    for (auto &[k, v] : env) {
      o << "\n    ";
      build_fn(o, v);
    }
  }

  void build_undefs() const {
    const auto ename = undefs_path();
    std::cerr << "generating " << ename.string() << std::endl;

    strmap exps;
    visit(wasm_env, exps);

    std::ofstream exp{ename};
    for (const auto &[k, v] : exps) {
      exp << k << "\n";
    }
  }

  void build_wasm() const {
    build_undefs();

    const auto fdir =
        std::filesystem::path{impl::current_target()->build_folder()};
    const auto fname = (fdir / exe_name()).replace_extension("js");

    std::cerr << "javascripting " << fname.string() << std::endl;
    std::ofstream o{fname};

    o << R"(function ecow(options) {
  var ecow_buffer;
  var ecow_globals = {};

  var name = ")" +
             name() + R"(";
  var inits = [)";
    build_inits(o);
    o << R"(
  ];
  var env = {)";
    build_env(o);
    o << R"(
  };
)";
    const auto me = std::filesystem::path{__FILE__}.parent_path();
    o << std::ifstream(me / "ecow.js").rdbuf();
    o << "}";
  }

protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target()->app_exe_name(name());
  }

  void build_self() const override {
    if (target_supports(webassembly))
      build_wasm();

    exe::build_self();

    const auto res_fld = impl::current_target()->resource_path(name());
    for (const auto &res : m_resources) {
      impl::run_copy(res, res_fld / res);
    }
  };

public:
  explicit app(const std::string &name) : exe{name} {}

  void add_resource(const std::string &name) { m_resources.push_back(name); }

  [[nodiscard]] strset link_flags() const override {
    auto res = exe::link_flags();

    if (target_supports(webassembly)) {
      res.insert("-Wl,--allow-undefined-file=" + undefs_path().string());
      res.insert("-mexec-model=reactor");
      res.insert("-flto");
      res.insert("-Wl,--lto-O3");

      strmap env;
      visit(export_syms, env);
      for (auto &[k, v] : env)
        res.insert("-Wl,--export=" + k);
    }

    return res;
  }
};
} // namespace ecow
