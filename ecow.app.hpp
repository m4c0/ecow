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
    add_link_flag("-mexec-model=reactor");
    add_link_flag("-flto");
    add_link_flag("-Wl,--lto-O3");

    std::cerr << "javascripting " << fname.string() << " and " << ename.string()
              << std::endl;
    std::ofstream o{fname};
    std::ofstream exp{ename};

    strmap env;
    visit(webassembly, env);

    o << R"(function ecow(options) {
  var ecow_buffer;
  var ecow_globals = {};

  function dump(fn, iovs, iovs_len, nwritten) {
    const view = new DataView(ecow_buffer);
    const decoder = new TextDecoder()
    var written = 0;
    var text = ''

    for (var i = 0; i < iovs_len; i++) {
      const ptr = iovs + i * 8;
      const buf = view.getUint32(ptr, true);
      const buf_len = view.getUint32(ptr + 4, true);
      text += decoder.decode(new Uint8Array(ecow_buffer, buf, buf_len));
      written += buf_len;
    }

    view.setUint32(nwritten, written, true);
    fn(text);
    return 0;
  }

  const imp = {
    env: {)";
    for (auto &[k, v] : env) {
      if (v == "") {
        add_link_flag("-Wl,--export=" + k);
        continue;
      }
      o << "\n          " << k << ": " << v << ",";
      exp << k << "\n";
    }
    o << R"(
    },
    wasi_snapshot_preview1: new Proxy({
      clock_time_get : (id, precision, out) => {
        if (id != 0) console.log("Unsupported clock type", id);
        var arr = new BigUint64Array(ecow_buffer, out, 3);
        arr[0] = BigInt(Date.now() * 1000000);
      }, 
      fd_close : (fd) => 0,
      fd_fdstat_get : (fd, stat) => 0,
      fd_write : (fd, iovs, iovs_len, nwritten) => {
        switch (fd) {
          case 1: return dump(console.log, iovs, iovs_len, nwritten);
          case 2: return dump(console.error, iovs, iovs_len, nwritten);
          default: return 24;
        }
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
    obj.instance.exports._initialize();
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

  void build_self() override {
    if (target_supports(webassembly))
      build_wasm();

    const auto res_fld = impl::current_target()->resource_path(name());
    for (const auto &res : m_resources) {
      impl::run_copy(res, res_fld / res);
    }

    exe::build_self();
  };

public:
  explicit app(const std::string &name) : exe{name} {}

  void add_resource(const std::string &name) { m_resources.push_back(name); }
};
} // namespace ecow
