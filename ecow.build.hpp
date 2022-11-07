#pragma once

#ifdef __APPLE__
#include "ecow.apple.hpp"
#elif _WIN32
#include "ecow.win32.hpp"
#else
#include "ecow.linux.hpp"
#endif
#include "ecow.droid.hpp"
#include "ecow.unit.hpp"

#include <iostream>

namespace ecow::impl {
template <typename T, typename... Args>
[[nodiscard]] static inline bool build(unit &u, Args &&...args) {
  auto &tgt = impl::current_target();
  tgt.reset(new T{std::forward<Args>(args)...});
  std::filesystem::create_directories(tgt->build_folder());
  return u.build();
}
[[nodiscard]] static inline bool run_main(unit &u, int argc, char **argv) {
  auto args = std::span{argv, static_cast<size_t>(argc)}.subspan(1);
  if (args.empty()) {
    return impl::build<impl::host_target>(u);
  }
  for (auto arg : args) {
    using namespace std::string_view_literals;
    if (arg == "clean"sv) {
      std::cerr << "Removing 'out'" << std::endl;
      std::filesystem::remove_all("out");
      continue;
    }
    if (arg == "android"sv) {
      if (!build<android_target>(u, "aarch64-none-linux-android26") ||
          !build<android_target>(u, "armv7-none-linux-androideabi26") ||
          !build<android_target>(u, "i686-none-linux-android26") ||
          !build<android_target>(u, "x86_64-none-linux-android26"))
        return false;
      continue;
    }
#ifdef __APPLE__
    return build<host_target>(u, arg);
#else
    std::cerr << "I don't know how to do '" << arg << "'" << std::endl;
    return false;
#endif
  }
  return true;
}
} // namespace ecow::impl

namespace ecow {
[[nodiscard]] static inline int run_main(unit &u, int argc, char **argv) {
  try {
    return impl::run_main(u, argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "Unexpected failure: " << e.what() << std::endl;
    return 1;
  }
}
} // namespace ecow
