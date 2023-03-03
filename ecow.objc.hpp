#pragma once

#include "ecow.unit.hpp"

namespace ecow {
class objc : public unit {
protected:
  void build_self() const override {
    if (!target_supports(objective_c))
      return;

    const auto mod_cache_path =
        impl::current_target()->module_cache_path().string();

    impl::clang{name(), obj_name(name())}
        .add_arg("-c")
        .add_arg("-fmodules")
        .add_arg("-fmodules-cache-path=" + mod_cache_path)
        .with_deps()
        .run();
  }
  [[nodiscard]] virtual pathset self_objects() const override {
    return target_supports(objective_c) ? unit::self_objects() : pathset{};
  }

public:
  explicit objc(const std::string &name) : unit{name + ".mm"} {}

  [[nodiscard]] virtual strset link_flags() const override {
    return target_supports(objective_c) ? unit::link_flags() : strset{};
  }
};
} // namespace ecow
