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
    env,
    js: { 
      mem: new WebAssembly.Memory({
        initial: 10,
        maximum: 100,
        shared: true,
      }),
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
    ecow_exports = obj.instance.exports;

    obj.instance.exports._initialize();
    for (var i in inits) {
      inits[i](obj);
    }
    return obj;
  }
  return fetch(options.base_dir + "/" + name + ".wasm")
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imp))
    .then(obj => ({
      exports: obj.instance.exports,
      start: () => start(obj),
    }));
