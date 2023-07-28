#pragma once

#ifdef __APPLE__
#include "ecow.apple.hpp"
#elif _WIN32
#include "ecow.win32.hpp"
#else
#include "ecow.linux.hpp"
#endif
#include "ecow.deps.hpp"
#include "ecow.droid.hpp"
#include "ecow.unit.hpp"
#include "ecow.wasm.hpp"

#include <iostream>

namespace ecow::impl {
const char *argv0;

template <typename T, typename... Args>
static inline void build(unit &u, Args &&...args) {
  T tgt{std::forward<Args>(args)...};
  std::filesystem::create_directories(tgt.build_folder());

  u.build();
}
static inline void run_main(unit &u, int argc, char **argv) {
  argv0 = argv[0];

  auto args = std::span{argv, static_cast<size_t>(argc)}.subspan(1);
  if (args.empty()) {
    build<host_target>(u);
    return;
  }
  for (auto arg : args) {
    using namespace std::string_view_literals;
    if (arg == "clean"sv) {
      std::cerr << "Removing 'out'" << std::endl;
      std::filesystem::remove_all("out");
      continue;
    }
    if (arg == "android"sv) {
      build<android_target>(u, "aarch64-none-linux-android26");
      build<android_target>(u, "armv7-none-linux-androideabi26");
      build<android_target>(u, "i686-none-linux-android26");
      build<android_target>(u, "x86_64-none-linux-android26");
      continue;
    }
    if (arg == "wasm"sv) {
      build<wasm_target>(u);
      continue;
    }
#ifdef __APPLE__
    if (arg == "ios"sv) {
      build<iphone_target>(u);
      build<iphonesimulator_target>(u);
      continue;
    } else if (arg == "apple"sv) {
      build<host_target>(u);
      build<iphone_target>(u);
      build<iphonesimulator_target>(u);
      continue;
    } else if (arg == "iphoneos"sv) {
      build<iphone_target>(u);
      continue;
    } else if (arg == "iphonesimulator"sv) {
      build<iphonesimulator_target>(u);
      continue;
    } else if (arg == "macosx"sv) {
      build<host_target>(u);
      continue;
    }
#elif _WIN32
    if (arg == "windows"sv) {
      build<host_target>(u);
      continue;
    }
#else
    if (arg == "linux"sv) {
      build<host_target>(u);
      continue;
    }
#endif
    using namespace std::string_literals;
    throw std::runtime_error("I don't know how to do '"s + arg + "'");
  }
}
} // namespace ecow::impl

namespace ecow {
[[nodiscard]] static inline int run_main(unit &u, int argc, char **argv) {
  try {
    impl::run_main(u, argc, argv);
    return 0;
  } catch (const impl::clang_failed &e) {
    std::cerr << "`clang` failed to run. Command:" << std::endl
              << e.what() << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Unexpected failure: " << e.what() << std::endl;
    return 1;
  }
}
[[nodiscard]] static inline int run_main(std::shared_ptr<unit> u, int argc,
                                         char **argv) {
  return run_main(*u, argc, argv);
}
} // namespace ecow
