#pragma once

namespace ecow::impl {
[[nodiscard]] static inline bool build(unit &u, impl::target t) {
  auto &tgt = impl::current_target();
  tgt = t;
  std::filesystem::create_directories(tgt.build_folder());
  return u.build();
}
} // namespace ecow::impl
namespace ecow {
[[nodiscard]] static inline int run_main(unit &u, int argc, char **argv) {
  auto args = std::span{argv, static_cast<size_t>(argc)}.subspan(1);
  if (args.empty()) {
    return impl::build(u, {}) ? 0 : 1;
  }
  for (auto arg : args) {
    using namespace std::string_view_literals;
    if (arg == "clean"sv) {
      u.clean();
      continue;
    }
    if (arg == "android"sv) {
      continue;
    }
#ifdef __APPLE__
    if (!impl::build(u, {arg}))
      return 1;
#else
    std::cerr << "I don't know how to do '" << arg << "'" << std::endl;
    return 1;
#endif
  }
  return 0;
}
} // namespace ecow
