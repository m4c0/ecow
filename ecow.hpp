#pragma once

#include "ecow.app.hpp"
#include "ecow.exe.hpp"
#include "ecow.mod.hpp"
#include "ecow.sys.hpp"

namespace ecow {
[[nodiscard]] static inline int run_main(unit &u, int argc, char **argv) {
  auto args = std::span{argv, static_cast<size_t>(argc)};
  switch (args.size()) {
  case 0:
    std::terminate();
  case 1:
    break;
  case 2:
    using namespace std::string_view_literals;
    if (args[1] == "clean"sv) {
      u.clean();
      return 0;
    }
    // TODO: if arg is android
#ifdef __APPLE__
    impl::current_target() = {argv[1]};
    break;
#endif
  default:
    std::cerr << "I don't know how to do that" << std::endl;
    return 1;
  }

  return u.build() ? 0 : 1;
}
} // namespace ecow
