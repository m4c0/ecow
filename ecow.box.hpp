#pragma once
#include "ecow.deps.hpp"
#include "ecow.mod.hpp"
#include "ecow.unit.hpp"

namespace ecow {
class box : public seq {
  std::set<std::string> m_cache{};

  void calculate_deps_of(const std::string &n) {
    if (deps::has(n))
      return;

    if (!std::filesystem::exists(n + ".cppm"))
      return;

    auto m = create<mod>(n);
    m->calculate_deps();

    for (auto &d : deps::of(n)) {
      calculate_deps_of(d);
    }

    add_unit<mod>(n);
  }

  virtual void calculate_self_deps() override {
    if (m_cache.size() > 0)
      return;

    calculate_deps_of(name());
  }

public:
  using seq::seq;
};
} // namespace ecow
