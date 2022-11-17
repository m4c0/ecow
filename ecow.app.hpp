#pragma once

#include "ecow.exe.hpp"

#include <fstream>
#include <numeric>

namespace ecow {
class app : public exe {
  std::vector<std::string> m_resources;

  void build_wasm() {
    const auto fdir =
        std::filesystem::path{impl::current_target()->build_folder()};
    const auto fname = (fdir / exe_name()).replace_extension("js");
    const auto ename = (fdir / exe_name()).replace_extension("exports");

    add_link_flag("-Wl,--allow-undefined-file=" + ename.string());

    std::cerr << "javascripting " << fname.string() << " and " << ename.string()
              << std::endl;
    std::ofstream o{fname};
    std::ofstream exp{ename};

    strmap env;
    visit(webassembly, env);

    o << R"(
    function ecow(options) {
      var ecow_buffer;
      const imp = {
        env: {)";
    for (auto &[k, v] : env) {
      o << "\n          " << k << ": " << v << ",";
      exp << k << "\n";
    }
    o << R"(
        },
        wasi_snapshot_preview1: new Proxy({
            clock_time_get : (id, precision, out) => {
            if (id != 0) console.log("Unsupported clock type", id);
            var arr = new BigUint64Array(obj.instance.exports.memory.buffer, out, 3);
            arr[0] = BigInt(Date.now() * 1000000);
          }, 
          proc_exit : () => { throw "oops" },
          ...options.extra_syscalls,
        }, {
          get(obj, prop) {
            return prop in obj ? obj[prop] : (... args) => {
              console.log(prop, ... args);
              throw prop + " is not defined";
            };
          },
        }),
      };
      function start(obj) {
        ecow_buffer = obj.instance.exports.memory.buffer;
        obj.instance.exports._start();
        return obj;
      }
      return fetch(options.base_dir + "/)" +
             name() + R"(.wasm")
        .then(response => response.arrayBuffer())
        .then(bytes => WebAssembly.instantiate(bytes, imp))
        .then(obj => ({
          exports: obj.instance.exports,
          start: () => start(obj),
        }));
    }
)";
  }

protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target()->app_exe_name(name());
  }

public:
  explicit app(const std::string &name) : exe{name} {}

  void add_resource(const std::string &name) { m_resources.push_back(name); }

  void build() override {
    if (target_supports(webassembly))
      build_wasm();

    const auto res_fld = impl::current_target()->resource_path(name());
    for (const auto &res : m_resources) {
      impl::run_copy(res, res_fld / res);
    }

    exe::build();
  };
};
} // namespace ecow
