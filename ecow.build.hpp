#pragma once

#ifdef __APPLE__
#include "ecow.apple.hpp"
#elif _WIN32
#include "ecow.win32.hpp"
#else
#include "ecow.linux.hpp"
#endif
#include "ecow.droid.hpp"

namespace ecow::impl {
template <typename T, typename... Args>
[[nodiscard]] static inline bool build(unit &u, Args &&...args) {
  auto &tgt = impl::current_target();
  tgt.reset(new T{std::forward<Args>(args)...});
  std::filesystem::create_directories(tgt->build_folder());
  return u.build();
}
} // namespace ecow::impl
namespace ecow {
[[nodiscard]] static inline int run_main(unit &u, int argc, char **argv) {
  auto args = std::span{argv, static_cast<size_t>(argc)}.subspan(1);
  if (args.empty()) {
    return impl::build<impl::host_target>(u) ? 0 : 1;
  }
  for (auto arg : args) {
    using namespace std::string_view_literals;
    if (arg == "clean"sv) {
      std::cerr << "Removing 'out'" << std::endl;
      std::filesystem::remove_all("out");
      continue;
    }
    if (arg == "android"sv) {
      if (!impl::build<impl::android_target>(u, "aarch64-none-linux-android26"))
        return 1;
      if (!impl::build<impl::android_target>(u,
                                             "armv7-none-linux-androideabi26"))
        return 1;
      if (!impl::build<impl::android_target>(u, "i686-none-linux-android26"))
        return 1;
      if (!impl::build<impl::android_target>(u, "x86_64-none-linux-android26"))
        return 1;
      continue;
    }
#ifdef __APPLE__
    if (!impl::build<impl::host_target>(u, arg))
      return 1;
#else
    std::cerr << "I don't know how to do '" << arg << "'" << std::endl;
    return 1;
#endif
  }
  return 0;
}
} // namespace ecow
