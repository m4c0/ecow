#pragma once

#include "ecow.exe.hpp"

#include <fstream>
#include <numeric>

namespace ecow {
class app : public exe {
protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target()->app_exe_name(name());
  }

public:
  explicit app(const std::string &name) : exe{name} {}

  void build() override {
    exe::build();

    if (target_supports(webassembly)) {
      const auto fname =
          std::filesystem::path{exe_name()}.replace_extension("js");
      std::cerr << "javascripting " << fname.string() << std::endl;
      std::ofstream o{impl::current_target()->build_folder() / fname};

      strmap env;
      visit(webassembly, env);

      o << R"(
    function ecow() {
      const imp = {
        env: {)";
      for (auto &[k, v] : env) {
        o << "\n          " << k << ": " << v << ",";
      }
      o << R"(
        },
        wasi_snapshot_preview1: new Proxy({
            clock_time_get : (id, precision, out) => {
            if (id != 0) console.log("Unsupported clock type", id);
            var arr = new BigUint64Array(obj.instance.exports.memory.buffer, out, 3);
            arr[0] = BigInt(Date.now() * 1000000);
          }, 
          proc_exit : () => { throw "oops" }
        }, {
          get(obj, prop) {
            return prop in obj ? obj[prop] : (... args) => {
              console.log(prop, ... args);
              throw prop + " is not defined";
            };
          },
        }),
      };
      return fetch(")" +
               name() + R"(.wasm")
        .then(response => response.arrayBuffer())
        .then(bytes => WebAssembly.instantiate(bytes, imp));
        .then(obj => {obj.instance.exports._initialize(); return obj;});
    }
)";
    }
  };
};
} // namespace ecow
